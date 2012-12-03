#include "sc_helper.h"
#include "sc_memory_headers.h"

#include <glib.h>
#include "string.h"

// sc-helper initialization flag
sc_bool sc_helper_is_initialized = SC_FALSE;

sc_char **keynodes_str = 0;
sc_addr *keynodes = 0;

sc_result resolve_nrel_system_identifier()
{
    sc_addr *results = 0;
    sc_uint32 results_count = 0;
    sc_stream *stream = sc_stream_memory_new(keynodes_str[SC_KEYNODE_NREL_SYSTEM_IDENTIFIER],
                                             sizeof(sc_uchar) * strlen(keynodes_str[SC_KEYNODE_NREL_SYSTEM_IDENTIFIER]),
                                             SC_STREAM_READ, SC_FALSE);
    sc_uint32 i = 0;
    sc_iterator5 *it = 0;
    sc_bool founded = SC_FALSE;
    sc_addr addr1, addr2;

    // try to find nrel_system_dentifier strings
    if (sc_memory_find_links_with_content(stream, &results, &results_count) == SC_OK)
    {
        for (i = 0; i < results_count; i++)
        {
            it = sc_iterator5_a_a_f_a_a_new(sc_type_node | sc_type_const | sc_type_node_norole,
                                            sc_type_arc_common | sc_type_const,
                                            results[i],
                                            sc_type_arc_pos_const_perm,
                                            sc_type_const | sc_type_node_norole);

            while (sc_iterator5_next(it))
            {

                addr1 = sc_iterator5_value(it, 0);
                addr2 = sc_iterator5_value(it, 4);
                // comare begin sc-element and attribute, they must be equivalent
                if (SC_ADDR_IS_EQUAL(addr1, addr2))
                {
                    if (founded == SC_FALSE)
                    {
                        keynodes[SC_KEYNODE_NREL_SYSTEM_IDENTIFIER] = addr1;
                        founded = SC_TRUE;
                    }else
                    {
                        sc_iterator5_free(it);
                        sc_stream_free(stream);
                        g_free(results);
                        g_error("There are more then one sc-elements with system identifier nrel_system_identifier ");
                        return SC_ERROR;
                    }
                }
            }

            sc_iterator5_free(it);
        }

        g_free(results);
    }else
        return SC_ERROR;

    sc_stream_free(stream);

    return SC_OK;
}

void _init_keynodes_str()
{
    gsize bytes_read = 0, bytes_written = 0;
    sc_uint32 i = 0;

    keynodes_str = g_new0(gchar*, SC_KEYNODE_COUNT);
    keynodes_str[SC_KEYNODE_NREL_SYSTEM_IDENTIFIER] = g_locale_to_utf8("nrel_system_identifier", -1, &bytes_read, &bytes_written, 0);


    // check for errors
    for (i = 0; i < (sc_uint32)SC_KEYNODE_COUNT; ++i)
    {
        if (keynodes_str[(sc_keynode)i] == nullptr)
            g_error("Error to create string representation of keynode: %d", i);
    }

}

void _destroy_keynodes_str()
{
    sc_uint32 i = 0;

    for (i = 0; i < (sc_uint32)SC_KEYNODE_COUNT; ++i)
        g_free(keynodes_str[i]);

    g_free(keynodes_str);
}

sc_result sc_helper_init()
{
    g_message("Initialize sc-helper");

    _init_keynodes_str();

    keynodes = g_new0(sc_addr, SC_KEYNODE_COUNT);

    if (resolve_nrel_system_identifier() != SC_OK)
    {
        g_error("Can't resovle nrel_system_identifier node");
        return SC_ERROR;
    }

    sc_helper_is_initialized = SC_TRUE;

    return SC_OK;
}

void sc_helper_shutdown()
{
    g_message("Shutdown sc-helper");

    g_free(keynodes);
    _destroy_keynodes_str();
}

sc_result sc_helper_find_element_by_system_identifier(sc_char* data, sc_uint32 len, sc_addr *result_addr)
{
    sc_addr *results = 0;
    sc_uint32 results_count = 0;
    sc_stream *stream = 0;
    sc_uint32 i = 0;
    sc_iterator5 *it = 0;
    sc_bool founded = SC_FALSE;

    g_assert(sc_helper_is_initialized == SC_TRUE);
    g_assert(keynodes != 0);

    // try to find sc-links with that contains system identifier value
    stream = sc_stream_memory_new(data, sizeof(sc_char) * len, SC_STREAM_READ, SC_FALSE);
    if (sc_memory_find_links_with_content(stream, &results, &results_count) == SC_OK)
    {
        for (i = 0; i < results_count; i++)
        {
            it = sc_iterator5_a_a_f_a_f_new(0,
                                            sc_type_arc_pos_const_perm,
                                            results[i],
                                            sc_type_arc_pos_const_perm,
                                            keynodes[SC_KEYNODE_NREL_SYSTEM_IDENTIFIER]);
            if (sc_iterator5_next(it))
            {
                if (founded == SC_FALSE)
                {
                    founded = SC_TRUE;
                    *result_addr = sc_iterator5_value(it, 0);
                }else
                {
                    // don't foget to free allocated memory before return error
                    sc_iterator5_free(it);
                    sc_stream_free(stream);
                    g_free(results);
                    return SC_ERROR_INVALID_STATE;
                }
            }

            sc_iterator5_free(it);
        }

        g_free(results);
    }

    sc_stream_free(stream);

    return SC_OK;
}

sc_result sc_helper_set_system_identifier(sc_addr addr, sc_char* data, sc_uint32 len)
{
    sc_iterator5 *it5 = 0;
    sc_addr *results = 0;
    sc_uint32 results_count = 0;
    sc_stream *stream = 0;
    sc_uint32 i = 0;
    sc_addr idtf_addr, arc_addr;

    SC_ADDR_MAKE_EMPTY(idtf_addr)
    g_assert(keynodes != 0);

    // try to find sc-links with that contains system identifier value
    stream = sc_stream_memory_new(data, sizeof(sc_char) * len, SC_STREAM_READ, SC_FALSE);
    if (sc_memory_find_links_with_content(stream, &results, &results_count) == SC_OK)
    {
        for (i = 0; i < results_count; i++)
        {
            it5 = sc_iterator5_a_a_f_a_f_new(0,
                                            sc_type_arc_pos_const_perm,
                                            results[i],
                                            sc_type_arc_pos_const_perm,
                                            keynodes[SC_KEYNODE_NREL_SYSTEM_IDENTIFIER]);
            if (sc_iterator5_next(it5))
            {
                idtf_addr = results[i];

                // don't foget to free allocated memory before return error
                sc_iterator5_free(it5);
                sc_stream_free(stream);
                g_free(results);
                return SC_ERROR_INVALID_PARAMS;
            }

            sc_iterator5_free(it5);
        }

        g_free(results);
    }

    // if there are no sc-link that contains identifier, then create it
    if (SC_ADDR_IS_EMPTY(idtf_addr))
    {
        idtf_addr = sc_memory_link_new();
        if (sc_memory_set_link_content(idtf_addr, stream) != SC_OK)
        {
            sc_stream_free(stream);
            return SC_ERROR;
        }
    }

    // we doesn't need link data anymore
    sc_stream_free(stream);

    // first of all try to find if specified identifier is already used
    it5 = sc_iterator5_f_a_a_a_f_new(addr,
                                     0,
                                     sc_type_link,
                                     sc_type_arc_pos_const_perm,
                                     keynodes[SC_KEYNODE_NREL_SYSTEM_IDENTIFIER]);

    // if specified sc-element already have system identifier, then change it
    if (sc_iterator5_next(it5) == SC_TRUE)
        sc_memory_element_free(sc_iterator5_value(it5, 1));

    // setup new system identifier
    arc_addr = sc_memory_arc_new(sc_type_arc_common, addr, idtf_addr);
    if (SC_ADDR_IS_EMPTY(arc_addr))
        return SC_ERROR;

    arc_addr = sc_memory_arc_new(sc_type_arc_pos_const_perm, keynodes[SC_KEYNODE_NREL_SYSTEM_IDENTIFIER], arc_addr);
    if (SC_ADDR_IS_EMPTY(arc_addr))
        return SC_ERROR;

    return SC_OK;
}

sc_result sc_helper_get_keynode(sc_keynode keynode, sc_addr *keynode_addr)
{
    if ((sc_helper_is_initialized == SC_FALSE) || (keynodes == nullptr))
        return SC_ERROR;

    *keynode_addr = keynodes[(sc_uint32)keynode];

    return SC_TRUE;
}
