
#include <archive.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "books-epub.h"

G_DEFINE_TYPE(BooksEpub, books_epub, G_TYPE_OBJECT)

#define BOOKS_EPUB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_EPUB, BooksEpubPrivate))

static gboolean content_exists (BooksEpubPrivate *priv, const gchar *pathname);
static gchar *get_content (BooksEpubPrivate *priv, const gchar *pathname);
static gchar *get_opf_path (BooksEpubPrivate *priv);
static void populate_document_spine (BooksEpubPrivate *priv);

GQuark
books_epub_error_quark (void)
{
    return g_quark_from_static_string ("books-epub-error-quark");
}

struct _BooksEpubPrivate {
    GHashTable  *content;    /* Maps from path name (gchar *) to actual content (gpointer) */
    GList       *documents;
    GList       *current;
};


BooksEpub *
books_epub_new (void)
{
    return BOOKS_EPUB (g_object_new (BOOKS_TYPE_EPUB, NULL));
}

gboolean
books_epub_open (BooksEpub *epub,
                 const gchar *filename,
                 GError **error)
{
    BooksEpubPrivate *priv;
    struct archive *arch;
    struct archive_entry *entry;
    gint result;

    g_return_if_fail (BOOKS_IS_EPUB (epub) && filename != NULL);
    
    arch = archive_read_new ();
    archive_read_support_filter_all (arch);
    archive_read_support_format_zip (arch);
    result = archive_read_open_filename (arch, filename, 10240);

    if (result != ARCHIVE_OK) {
        g_set_error (error, BOOKS_EPUB_ERROR, BOOKS_EPUB_ERROR_INVALID_ARCHIVE_FORMAT,
                     "`%s' is not a valid EPUB archive", filename); 
        return FALSE;
    }

    priv = epub->priv;

    while (archive_read_next_header (arch, &entry) == ARCHIVE_OK) {
        const void *data;
        gchar *pathname;
        gsize size;
        gint64 offset;

        pathname = (gchar *) archive_entry_pathname (entry);
        archive_read_data_block (arch, &data, &size, &offset);
        g_hash_table_insert (priv->content, 
                             g_strdup (pathname), 
                             g_memdup (data, size));
    }
    
    archive_read_free (arch);

    if (!content_exists (priv, "META-INF/container.xml")) {
        g_set_error (error, BOOKS_EPUB_ERROR, BOOKS_EPUB_ERROR_NO_META_DATA,
                     "No meta data found in `%s'", filename);
        return FALSE;
    }

    populate_document_spine (priv);
    priv->current = g_list_first (priv->documents);

    return TRUE;
}

gchar *
books_epub_get_content (BooksEpub *epub)
{
    BooksEpubPrivate *priv;
    gchar *item;

    g_return_if_fail (BOOKS_IS_EPUB (epub));
    priv = epub->priv;
    g_print ("retrieving content of %s\n", (gchar *) priv->current->data);
    return get_content (priv, priv->current->data);
}

void
books_epub_next (BooksEpub *epub)
{
    BooksEpubPrivate *priv;

    g_return_if_fail (BOOKS_IS_EPUB (epub));
    priv = epub->priv;

    if (priv->current != g_list_last (priv->documents))
        priv->current = g_list_next (priv->current);
}

void
books_epub_previous (BooksEpub *epub)
{
    BooksEpubPrivate *priv;

    g_return_if_fail (BOOKS_IS_EPUB (epub));
    priv = epub->priv;

    if (priv->current != g_list_first (priv->documents))
        priv->current = g_list_previous (priv->current);
}

gboolean
books_epub_is_first (BooksEpub *epub)
{
    g_return_if_fail (BOOKS_IS_EPUB (epub));
    return epub->priv->current == g_list_first (epub->priv->documents);
}

gboolean
books_epub_is_last (BooksEpub *epub)
{
    g_return_if_fail (BOOKS_IS_EPUB (epub));
    return epub->priv->current == g_list_last (epub->priv->documents);
}

static gboolean
content_exists (BooksEpubPrivate *priv,
                const gchar *pathname)
{
    return g_hash_table_lookup (priv->content, pathname) != NULL;
}

static gchar *
get_content (BooksEpubPrivate *priv,
             const gchar *pathname)
{
    return (gchar *) g_hash_table_lookup (priv->content, pathname);
}

static gchar *
get_opf_path (BooksEpubPrivate *priv)
{
    gchar *container_data;
    xmlDoc *tree;
    xmlXPathContext *context;
    xmlXPathObject *object;
    xmlNode *node;
    gchar *path = NULL;

    container_data = get_content (priv, "META-INF/container.xml");
    tree = xmlParseDoc (container_data);

    if (tree == NULL)
        return NULL;

    context = xmlXPathNewContext (tree);

    xmlXPathRegisterNs (context, "c", "urn:oasis:names:tc:opendocument:xmlns:container");
    object = xmlXPathEvalExpression ("//c:container/c:rootfiles/c:rootfile",
                                     context);

    if (object == NULL) {
        g_error ("Could not evaluate xpath expression"); 
        goto get_opf_path_cleanup;
    }

    if (object->nodesetval != NULL)
        path = g_strdup (xmlGetProp (object->nodesetval->nodeTab[0], "full-path"));

get_opf_path_cleanup:
    xmlXPathFreeObject (object);
    xmlXPathFreeContext (context);
    xmlFreeDoc (tree);
    xmlCleanupParser ();

    return path;
}

static gchar *
get_document_item (xmlXPathContext *context, gchar *item_id)
{
    xmlXPathObject *object;
    gchar *expression;
    gchar *item_html = NULL;

    expression = g_strdup_printf ("//pkg:package/pkg:manifest/pkg:item[@id='%s']", item_id);
    object = xmlXPathEvalExpression (expression, context);

    if (object->nodesetval != NULL)
        item_html = g_strdup (xmlGetProp (object->nodesetval->nodeTab[0], "href"));

    g_free (expression);
    return item_html;
}

static void
populate_document_spine (BooksEpubPrivate *priv)
{
    gchar *opf_data;
    gchar *opf_path;
    gchar *opf_prefix;
    xmlDoc *tree;
    xmlXPathContext *context;
    xmlXPathObject *object;

    if (priv->documents != NULL) {
        g_list_free_full (priv->documents, g_free);
        priv->documents = NULL;
    }

    opf_path = get_opf_path (priv);
    opf_prefix = g_path_get_dirname (opf_path);
    opf_data = get_content (priv, opf_path);
    tree = xmlParseDoc (opf_data);
    context = xmlXPathNewContext (tree);

    xmlXPathRegisterNs (context, "pkg", "http://www.idpf.org/2007/opf");
    object = xmlXPathEvalExpression ("//pkg:package/pkg:spine/pkg:itemref", context);

    if (object->nodesetval != NULL) {
        guint i;

        for (i = 0; i < object->nodesetval->nodeNr; i++) {
            xmlNode *node;
            gchar *item_id;
            gchar *item;

            node = object->nodesetval->nodeTab[i];
            item_id = xmlGetProp (node, "idref");
            item = get_document_item (context, item_id);

            if (item != NULL) {
                gchar *full_path;
                full_path = g_strdup_printf ("%s/%s", opf_prefix, item);
                priv->documents = g_list_append (priv->documents, full_path);
            }
        } 
    }

    xmlXPathFreeObject (object);
    xmlXPathFreeContext (context);
    xmlFreeDoc (tree);
    xmlCleanupParser ();
    g_free (opf_path);
    g_free (opf_prefix);
}

static void
books_epub_dispose (GObject *object)
{
    G_OBJECT_CLASS (books_epub_parent_class)->dispose (object);
}

static void
books_epub_finalize (GObject *object)
{
    BooksEpubPrivate *priv;

    priv = BOOKS_EPUB_GET_PRIVATE (object);

    g_hash_table_destroy (priv->content);
    priv->content = NULL;

    if (priv->documents != NULL) {
        g_list_free_full (priv->documents, g_free);
        priv->documents = NULL;
    }

    G_OBJECT_CLASS (books_epub_parent_class)->finalize (object);
}

static void
books_epub_set_property (GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
books_epub_get_property(GObject *object,
                        guint property_id,
                        GValue *value,
                        GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void books_epub_class_init(BooksEpubClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = books_epub_set_property;
    gobject_class->get_property = books_epub_get_property;
    gobject_class->dispose = books_epub_dispose;
    gobject_class->finalize = books_epub_finalize;

    g_type_class_add_private(klass, sizeof(BooksEpubPrivate));
}

static void books_epub_init(BooksEpub *self)
{
    BooksEpubPrivate *priv;

    self->priv = priv = BOOKS_EPUB_GET_PRIVATE (self);
    priv->content = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           g_free, g_free);
    priv->documents = NULL;
}

