
#include <archive.h>
#include <archive_entry.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "books-epub.h"

G_DEFINE_TYPE(BooksEpub, books_epub, G_TYPE_OBJECT)

#define BOOKS_EPUB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_EPUB, BooksEpubPrivate))

static GError   *extract_archive            (BooksEpubPrivate *priv,
                                             const gchar *pathname,
                                             const gchar *path);
static gchar    *get_content                (BooksEpubPrivate *priv, const gchar *filename);
static gchar    *get_opf_path               (BooksEpubPrivate *priv);
static gchar    *get_cover_path             (BooksEpubPrivate *priv);
static gchar    *get_content_filename       (BooksEpubPrivate *priv, const gchar *filename);
static void      populate_document_spine    (BooksEpubPrivate *priv);
static gchar    *remove_uri_anchor          (const gchar *uri);

GQuark
books_epub_error_quark (void)
{
    return g_quark_from_static_string ("books-epub-error-quark");
}

struct _BooksEpubPrivate {
    GList   *documents;
    GList   *current;
    gchar   *path;
    gchar   *opf_path;
    gchar   *opf_prefix;
    gchar   *cover_path;
    xmlDoc  *opf_tree;
    xmlXPathContext *opf_xpath_context;
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
    gchar *basename;
    gchar *opf_data;
    GError *tmp_error = NULL;

    g_return_val_if_fail (BOOKS_IS_EPUB (epub) && filename != NULL, FALSE);

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

    priv->opf_path = get_opf_path (priv);
    priv->opf_prefix = g_path_get_dirname (priv->opf_path);
    opf_data = get_content (priv, priv->opf_path);
    priv->opf_tree = xmlParseDoc ((const xmlChar*) opf_data);
    g_free (opf_data);
    priv->opf_xpath_context = xmlXPathNewContext (priv->opf_tree);

    xmlXPathRegisterNs (priv->opf_xpath_context,
                        (const xmlChar *) "dc",
                        (const xmlChar *) "http://purl.org/dc/elements/1.1/");

    xmlXPathRegisterNs (priv->opf_xpath_context,
                        (const xmlChar *) "pkg",
                        (const xmlChar *) "http://www.idpf.org/2007/opf");

    populate_document_spine (priv);
    priv->cover_path = get_cover_path (priv);

    return TRUE;
}

const gchar *
books_epub_get_uri (BooksEpub *epub)
{
    BooksEpubPrivate *priv;

    g_return_val_if_fail (BOOKS_IS_EPUB (epub), NULL);
    priv = epub->priv;
    return priv->current != NULL ? priv->current->data : NULL;
}

const gchar *
books_epub_get_cover (BooksEpub *epub)
{
    g_return_val_if_fail (BOOKS_IS_EPUB (epub), NULL);
    return epub->priv->cover_path;
}

void
books_epub_set_uri (BooksEpub *epub,
                    const gchar *uri)
{
    BooksEpubPrivate *priv;
    gchar *normalized_uri;
    GList *result;

    g_return_if_fail (BOOKS_IS_EPUB (epub));

    priv = epub->priv;
    normalized_uri = remove_uri_anchor (uri);
    result = g_list_find_custom (priv->documents, normalized_uri, (GCompareFunc) g_strcmp0);

    if (result != NULL)
        priv->current = result;

    g_free (normalized_uri);
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
    g_return_val_if_fail (BOOKS_IS_EPUB (epub), FALSE);
    return epub->priv->current == g_list_first (epub->priv->documents);
}

gboolean
books_epub_is_last (BooksEpub *epub)
{
    g_return_val_if_fail (BOOKS_IS_EPUB (epub), FALSE);
    return epub->priv->current == g_list_last (epub->priv->documents);
}

const gchar *
books_epub_get_meta (BooksEpub *epub,
                     gchar *key)
{
    BooksEpubPrivate *priv;
    xmlXPathObject *object;
    gchar *expression;
    const gchar *value = NULL;

    g_return_val_if_fail (BOOKS_IS_EPUB (epub), NULL);

    priv = epub->priv;
    expression = g_strdup_printf ("//pkg:package/pkg:metadata/dc:%s", key);
    object = xmlXPathEvalExpression ((const xmlChar *) expression, priv->opf_xpath_context);

    if (!xmlXPathNodeSetIsEmpty (object->nodesetval)) {
        value = (const gchar *) xmlNodeListGetString (priv->opf_tree,
                                                      object->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
    }

    xmlXPathFreeObject (object);
    return value;
}

static gchar *
remove_uri_anchor (const gchar *uri)
{
    gchar *anchor;

    anchor = g_strrstr (uri, "#");

    if (anchor == NULL)
        return g_strdup (uri);
    else
        return g_strndup (uri, anchor - uri);
}

static gint
copy_archive_data (struct archive *ar, struct archive *aw)
{
    gint r;
    const void *buff;
    size_t size;
    gint64  offset;

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
             const gchar *filename)
{
    FILE *fp;
    gsize size, read;
    gchar *content = NULL;
    gchar *new_path;

    new_path = g_build_path (G_DIR_SEPARATOR_S, priv->path, filename, NULL);
    fp = fopen (new_path, "rb");

    if (fp != NULL) {
        fseek (fp, 0, SEEK_END);
        size = ftell (fp);
        fseek (fp, 0, SEEK_SET);
        content = g_malloc0 (size + 1);
        read = fread (content, 1, size, fp);
        g_assert (read == size);
        fclose (fp);
    }

    g_free (new_path);
    return content;
}

static gchar *
get_content_filename (BooksEpubPrivate *priv,
                      const gchar *filename)
{
    return g_build_path (G_DIR_SEPARATOR_S, priv->path, priv->opf_prefix, filename, NULL);
}

static gchar *
get_opf_path (BooksEpubPrivate *priv)
{
    gchar *container_data;
    xmlDoc *tree;
    xmlXPathContext *context;
    xmlXPathObject *object;
    gchar *path = NULL;

    container_data = get_content (priv, "META-INF/container.xml");
    tree = xmlParseDoc ((const xmlChar *) container_data);

    if (tree == NULL)
        return NULL;

    context = xmlXPathNewContext (tree);

    xmlXPathRegisterNs (context,
                        (const xmlChar *) "c",
                        (const xmlChar *) "urn:oasis:names:tc:opendocument:xmlns:container");

    object = xmlXPathEvalExpression ((const xmlChar *) "//c:container/c:rootfiles/c:rootfile",
                                     context);

    if (object == NULL) {
        g_error ("Could not evaluate xpath expression");
        goto get_opf_path_cleanup;
    }

    if (object->nodesetval != NULL)
        path = g_strdup ((const gchar *) xmlGetProp (object->nodesetval->nodeTab[0],
                                                     (const xmlChar *) "full-path"));

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
    object = xmlXPathEvalExpression ((const xmlChar *) expression, context);

    if (object->nodesetval != NULL)
        item_html = g_strdup ((const gchar *) xmlGetProp (object->nodesetval->nodeTab[0],
                                                          (const xmlChar *) "href"));

    g_free (expression);
    return item_html;
}

static void
populate_document_spine (BooksEpubPrivate *priv)
{
    xmlXPathObject *object;

    if (priv->documents != NULL) {
        g_list_free_full (priv->documents, g_free);
        priv->documents = NULL;
    }

    object = xmlXPathEvalExpression ((const xmlChar *) "//pkg:package/pkg:spine/pkg:itemref",
                                     priv->opf_xpath_context);

    if (object->nodesetval != NULL) {
        guint i;

        for (i = 0; i < object->nodesetval->nodeNr; i++) {
            xmlNode *node;
            gchar *item_id;
            gchar *item;

            node = object->nodesetval->nodeTab[i];
            item_id = (gchar *) xmlGetProp (node, (const xmlChar *) "idref");
            item = get_document_item (priv->opf_xpath_context, item_id);

            if (item != NULL) {
                gchar *filename;
                gchar *uri;
                GError *error = NULL;

                filename = get_content_filename (priv, item);
                uri = g_filename_to_uri (filename, NULL, &error);
                priv->documents = g_list_append (priv->documents, uri);
                g_free (filename);
            }
        }
    }

    priv->current = g_list_first (priv->documents);
    xmlXPathFreeObject (object);
}

static gchar *
get_cover_path (BooksEpubPrivate *priv)
{
    guint i;
    gchar *path = NULL;

    static const gchar *cover_ids[] = {
        "cover",
        "cover-image",
        "my-cover-image",
        "cover.jpeg",
        NULL,
    };

    for (i = 0; cover_ids[i] != NULL && path == NULL; i++) {
        xmlXPathObject *object;
        gchar *expression;

        expression = g_strdup_printf ("//pkg:package/pkg:manifest/pkg:item[@id='%s']", cover_ids[i]);
        object = xmlXPathEvalExpression ((const xmlChar *) expression, priv->opf_xpath_context);

        if (!xmlXPathNodeSetIsEmpty (object->nodesetval)) {
            xmlNode *node;
            gchar *href;

            node = object->nodesetval->nodeTab[0];
            href = (gchar *) xmlGetProp (node, (const xmlChar *) "href");
            path = get_content_filename (priv, href);
            g_free (href);
        }

        g_free (expression);
        xmlXPathFreeObject (object);
    }

    return path;
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

    if (priv->cover_path != NULL)
        g_free (priv->cover_path);

    if (priv->opf_prefix != NULL)
        g_free (priv->opf_prefix);

    if (priv->opf_xpath_context != NULL) {
        xmlXPathFreeContext (priv->opf_xpath_context);
        priv->opf_xpath_context = NULL;
    }

    if (priv->opf_tree != NULL) {
        xmlFreeDoc (priv->opf_tree);
        xmlCleanupParser ();
        priv->opf_tree = NULL;
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
    priv->documents = NULL;
    priv->path = NULL;
    priv->cover_path = NULL;
    priv->opf_prefix = NULL;
    priv->opf_tree = NULL;
    priv->opf_xpath_context = NULL;
}

