
#include <archive.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "books-epub.h"

G_DEFINE_TYPE(BooksEpub, books_epub, G_TYPE_OBJECT)

#define BOOKS_EPUB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_EPUB, BooksEpubPrivate))

static GError   *extract_archive            (BooksEpubPrivate *priv,
                                             const gchar *pathname,
                                             const gchar *path);
static gchar    *get_content                (BooksEpubPrivate *priv, const gchar *pathname);
static gchar    *get_opf_path               (BooksEpubPrivate *priv);
static void      populate_document_spine    (BooksEpubPrivate *priv);

GQuark
books_epub_error_quark (void)
{
    return g_quark_from_static_string ("books-epub-error-quark");
}

struct _BooksEpubPrivate {
    GList *documents;
    GList *current;
    gchar *path;
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
    gint result;
    gchar *basename;
    GError *tmp_error = NULL;

    g_return_if_fail (BOOKS_IS_EPUB (epub) && filename != NULL);

    priv = epub->priv;

    if (priv->path != NULL)
        g_free (priv->path);

    basename = g_path_get_basename (filename);

    priv->path = g_build_path (G_DIR_SEPARATOR_S,
                               g_get_user_cache_dir(),
                               "books",
                               basename,
                               NULL);

    g_free (basename);

    if (!g_file_test (priv->path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
        tmp_error = extract_archive (priv, filename, priv->path);

    if (tmp_error != NULL) {
        g_propagate_error (error, tmp_error);
        return FALSE;
    }

    populate_document_spine (priv);
    return TRUE;
}

gchar *
books_epub_get_uri (BooksEpub *epub)
{
    BooksEpubPrivate *priv;
    gchar *path;
    gchar *uri;
    GError *error = NULL;

    g_return_if_fail (BOOKS_IS_EPUB (epub));
    priv = epub->priv;
    path = g_build_path (G_DIR_SEPARATOR_S, priv->path, priv->current->data, NULL);
    uri = g_filename_to_uri (path, NULL, &error);

    if (error != NULL)
        g_warning ("%s\n", error->message);

    g_free (path);
    return uri;
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

static gint
copy_archive_data (struct archive *ar, struct archive *aw)
{
    gint r;
    const void *buff;
    size_t size;
    guint64  offset;

    for (;;) {
        r = archive_read_data_block(ar, &buff, &size, &offset);

        if (r == ARCHIVE_EOF)
            return ARCHIVE_OK;

        if (r != ARCHIVE_OK)
            return r;

        r = archive_write_data_block(aw, buff, size, offset);

        if (r != ARCHIVE_OK) {
            return r;
        }
    }
}

static GError *
extract_archive (BooksEpubPrivate *priv,
                 const gchar *filename,
                 const gchar *path)
{
    struct archive *arch;
    struct archive *ext;
    struct archive_entry *entry;
    gint flags, result;
    gchar *new_path = NULL;
    GError *error = NULL;

    arch = archive_read_new ();
    archive_read_support_filter_all (arch);
    archive_read_support_format_zip (arch);

    flags = ARCHIVE_EXTRACT_TIME |
            ARCHIVE_EXTRACT_PERM |
            ARCHIVE_EXTRACT_ACL  |
            ARCHIVE_EXTRACT_FFLAGS;

    ext = archive_write_disk_new ();
    archive_write_disk_set_options (ext, flags);
    archive_write_disk_set_standard_lookup (ext);

    result = archive_read_open_filename (arch, filename, 10240);

    if (result != ARCHIVE_OK) {
        g_set_error (&error, BOOKS_EPUB_ERROR, BOOKS_EPUB_ERROR_INVALID_ARCHIVE_FORMAT,
                     "`%s' is not a valid EPUB archive", filename);
        return FALSE;
    }

    for (;;) {
        result = archive_read_next_header (arch, &entry);

        if (result == ARCHIVE_EOF)
            break;

        if (result != ARCHIVE_OK) {
            g_set_error (&error, BOOKS_EPUB_ERROR, BOOKS_EPUB_ERROR_INVALID_ARCHIVE_FORMAT,
                         "`%s' is corrupted: %s", filename, archive_error_string (arch));
            goto extract_archive_cleanup;
        }

        new_path = g_build_path (G_DIR_SEPARATOR_S, path,
                                 archive_entry_pathname (entry), NULL);
        archive_entry_set_pathname (entry, new_path);

        result = archive_write_header (ext, entry);

        if (result != ARCHIVE_OK) {
            g_set_error (&error, BOOKS_EPUB_ERROR, BOOKS_EPUB_ERROR_INVALID_ARCHIVE_FORMAT,
                         "Could not write header: %s", archive_error_string (ext));
            goto extract_archive_cleanup;
        }
        else if (archive_entry_size (entry) > 0) {
            result = copy_archive_data (arch, ext);

            if (result != ARCHIVE_OK) {
                g_set_error (&error, BOOKS_EPUB_ERROR, BOOKS_EPUB_ERROR_INVALID_ARCHIVE_FORMAT,
                             "Could not write header: %s", archive_error_string (ext));
                goto extract_archive_cleanup;
            }
        }

        result = archive_write_finish_entry (ext);

        if (result != ARCHIVE_OK) {
            g_set_error (&error, BOOKS_EPUB_ERROR, BOOKS_EPUB_ERROR_INVALID_ARCHIVE_FORMAT,
                         "Could not finish entry: %s", archive_error_string (ext));
            goto extract_archive_cleanup;
        }

        g_free (new_path);
    }

extract_archive_cleanup:
    archive_read_close (arch);
    archive_read_free (arch);
    archive_write_close (ext);
    archive_write_free (ext);
    return error;
}

static gchar *
get_content (BooksEpubPrivate *priv,
             const gchar *pathname)
{
    FILE *fp;
    gsize size, read;
    gchar *content = NULL;
    gchar *new_path;

    new_path = g_build_path (G_DIR_SEPARATOR_S, priv->path, pathname, NULL);
    fp = fopen (new_path, "rb");

    if (fp != NULL) {
        fseek (fp, 0, SEEK_END);
        size = ftell (fp);
        fseek (fp, 0, SEEK_SET);
        content = g_malloc0 (size);
        read = fread (content, 1, size, fp);
        g_assert (read == size);
        fclose (fp);
    }

    g_free (new_path);
    return content;
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
                full_path = g_build_path (G_DIR_SEPARATOR_S, opf_prefix, item, NULL);
                priv->documents = g_list_append (priv->documents, full_path);
            }
        }
    }

    priv->current = g_list_first (priv->documents);

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

    if (priv->documents != NULL) {
        g_list_free_full (priv->documents, g_free);
        priv->documents = NULL;
    }

    if (priv->path != NULL)
        g_free (priv->path);

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
    priv->documents = NULL;
    priv->path = NULL;
}

