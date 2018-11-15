//
// Created by root on 11/14/18.
//

#include <stdio.h>
#include <memory.h>
#include <json/json.h>
#include <arpa/inet.h>
#include "ipdb.h"

int is_big_endian(void) {
    union {
        uint32_t i;
        char c[4];
    } e = {0x01000000};

    return e.c[0];
}

unsigned int l2b(unsigned int x) {
    return (((x >> 24) & 0x000000ff) | ((x >> 8) & 0x0000ff00) | ((x << 8) & 0x00ff0000) | ((x << 24) & 0xff000000));
}

ipdb_meta_data *parse_meta_data(const char *meta_json) {
    ipdb_meta_data *meta_data = (ipdb_meta_data *) malloc(sizeof(ipdb_meta_data));
    memset(meta_data, 0, sizeof(ipdb_meta_data));
    json_object *obj = json_tokener_parse(meta_json);
    json_object *value;
    json_object_object_get_ex(obj, "node_count", &value);
    meta_data->node_count = json_object_get_int(value);
    json_object_object_get_ex(obj, "total_size", &value);
    meta_data->total_size = json_object_get_int(value);
    json_object_object_get_ex(obj, "build", &value);
    meta_data->build_time = json_object_get_int64(value);
    json_object_object_get_ex(obj, "ip_version", &value);
    meta_data->ip_version = (short) json_object_get_int(value);
    json_object_object_get_ex(obj, "fields", &value);
    meta_data->fields_length = json_object_array_length(value);
    meta_data->fields = (char **) malloc(sizeof(char *) * meta_data->fields_length);
    for (int i = 0; i < meta_data->fields_length; ++i) {
        json_object *it = json_object_array_get_idx(value, i);
        meta_data->fields[i] = malloc(sizeof(char) * json_object_get_string_len(it) + 1);
        strcpy(meta_data->fields[i], json_object_get_string(it));
    }
    json_object_object_get_ex(obj, "languages", &value);
    meta_data->language_length = json_object_object_length(value);
    meta_data->language = (ipdb_meta_data_language *) malloc(
            sizeof(ipdb_meta_data_language) * meta_data->language_length);
    struct json_object_iterator language = json_object_iter_begin(value);
    for (int i = 0; i < meta_data->language_length; ++i) {
        strcpy(meta_data->language[i].name, json_object_iter_peek_name(&language));
        struct json_object *it = json_object_iter_peek_value(&language);
        meta_data->language[i].offset = json_object_get_int(it);
        json_object_iter_next(&language);
    }
    json_object_iter_end(value);
    return meta_data;
}

int ipdb_read_node(ipdb_reader *reader, int node, int index) {
    int off = node * 8 + index * 4;
    int tar = *(int *) &reader->data[off];
    return l2b((unsigned int) tar);
}

int ipdb_reader_new(const char *file, ipdb_reader **reader) {
    FILE *fd = fopen(file, "rb");
    if (!fd) {
        return ErrFileSize;
    }
    *reader = malloc(sizeof(ipdb_reader));
    ipdb_reader *rd = *reader;

    fseek(fd, 0, SEEK_END);
    long fsize = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    unsigned int meta_length = 0;
    fread(&meta_length, sizeof(meta_length), 1, fd);
    meta_length = is_big_endian() ? meta_length : l2b(meta_length);
    printf("is_big_endian: %d, file_size: %ld, meta_len: %d\n", is_big_endian(), fsize, meta_length);

    char *meta_json = (char *) malloc(meta_length + 1);
    meta_json[meta_length] = 0;
    fread(meta_json, sizeof(char), meta_length, fd);
    printf("meta: %s\n", meta_json);
    rd->meta = parse_meta_data(meta_json);
    free(meta_json);
    printf("build_time: %ld\n", rd->meta->build_time);
    printf("ip_version: %hi\n", rd->meta->ip_version);
    printf("node_count: %i\n", rd->meta->node_count);
    printf("total_size: %i\n", rd->meta->total_size);
    printf("fields: ");
    for (int i = 0; i < rd->meta->fields_length; ++i) {
        printf("%s ", rd->meta->fields[i]);
    }
    printf("\n");
    printf("language: ");
    for (int i = 0; i < rd->meta->language_length; ++i) {
        printf("%s: %d ", rd->meta->language[i].name, rd->meta->language[i].offset);
    }
    printf("\n");
    if (rd->meta->language_length == 0 || rd->meta->fields_length == 0) {
        return ErrMetaData;
    }

    if (fsize != (4 + meta_length + rd->meta->total_size)) {
        return ErrFileSize;
    }
    rd->file_size = (int) fsize;
    int data_len = (int) fsize - 4 - meta_length;
    rd->data = (unsigned char *) malloc(sizeof(unsigned char) * data_len);
    fread(rd->data, sizeof(unsigned char), (size_t) data_len, fd);
    rd->data_size = data_len;

    int node = 0;
    for (int i = 0; i < 96 && node < rd->meta->node_count; ++i) {
        if (i >= 80) {
            node = ipdb_read_node(rd, node, 1);
        } else {
            node = ipdb_read_node(rd, node, 0);
        }
    }
    rd->v4offset = node;

    fclose(fd);
    return ErrNoErr;
}

void ipdb_reader_free(ipdb_reader **reader) {
    if ((*reader)->meta) {
        ipdb_meta_data *meta_data = (*reader)->meta;
        for (int i = 0; i < meta_data->fields_length; ++i) {
            free(meta_data->fields[i]);
        }
        free(meta_data->fields);
        free(meta_data->language);
        free(meta_data);
    }
    if ((*reader)->data) {
        free((*reader)->data);
    }
    free(*reader);
    *reader = 0;
}

int ipdb_reader_is_ipv4_support(ipdb_reader *reader) {
    return (((int) reader->meta->ip_version) & IPv4) == IPv4;

}

int ipdb_reader_is_ipv6_support(ipdb_reader *reader) {
    return (((int) reader->meta->ip_version) & IPv6) == IPv6;

}

int ipdb_resolve(ipdb_reader *reader, int node, const char **bytes) {
    int resolved = node - reader->meta->node_count + reader->meta->node_count * 8;
    if (resolved >= reader->file_size) {
        return ErrDatabaseError;
    }

    int size = (reader->data[resolved] << 8) | reader->data[resolved + 2];
    if ((resolved + 2 + size) > reader->data_size) {
        return ErrDatabaseError;
    }
    *bytes = (const char *) reader->data + resolved + 2;
    return ErrNoErr;
}

int ipdb_search(ipdb_reader *reader, const u_char *ip, int bit_count, int *node) {
    *node = 0;

    if (bit_count == 32) {
        *node = reader->v4offset;
    } else {
        *node = 0;
    }

    for (int i = 0; i < bit_count; ++i) {
        if (*node > reader->meta->node_count) {
            break;
        }

        *node = ipdb_read_node(reader, *node,
                               ((0xFF & ((int) ip[i >> 3])) >> (unsigned int) (7 - (i % 8))) & 1);
    }

    if (*node > reader->meta->node_count) {
        return ErrNoErr;
    }
    return ErrDataNotExists;
}

int ipdb_find0(ipdb_reader *reader, const char *addr, const char **body) {
    int node = 0;
    int err = ErrNoErr;
    struct in_addr addr4;
    struct in6_addr addr6;
    if (inet_pton(AF_INET, addr, &addr4)) {
        if (!ipdb_reader_is_ipv4_support(reader)) {
            return ErrNoSupportIPv4;
        }
        err = ipdb_search(reader, (const u_char *) &addr4.s_addr, 32, &node);
        if (err != ErrNoErr) {
            return err;
        }
    } else if (inet_pton(AF_INET6, addr, &addr6)) {
        if (!ipdb_reader_is_ipv6_support(reader)) {
            return ErrNoSupportIPv6;
        }
        err = ipdb_search(reader, (const u_char *) &addr6.s6_addr, 128, &node);
        if (err != ErrNoErr) {
            return err;
        }
    } else {
        return ErrIPFormat;
    }
    err = ipdb_resolve(reader, node, body);
    return err;
}

int split(const char *src, const char sp, char ***result) {
    int sum = 1;
    const char *c = src;
    while (*c) {
        if (*c == '\t')++sum;
        ++c;
    }
    *result = malloc(sizeof(char *) * sum);
    int i = 0;
    size_t pos = 0;
    c = src;
    while (*(c + pos)) {
        if (*(c + pos) == sp) {
            (*result)[i] = malloc(pos + 1);
            strncpy((*result)[i], c, pos);
            (*result)[i][pos] = 0;
            ++i;
            c = c + pos + 1;
            pos = 0;
            continue;
        }
        ++pos;
    }
    size_t len = strlen(c);
    (*result)[i] = malloc(len + 1);
    if (len) strncpy((*result)[i], c, len);
    (*result)[i][len] = 0;
    return sum;
}

int ipdb_find1(ipdb_reader *reader, const char *addr, const char *language, ipdb_string_chain **body) {
    int err;
    int off = -1;
    for (int i = 0; i < reader->meta->language_length; ++i) {
        if (strcmp(language, reader->meta->language[i].name) == 0) {
            off = reader->meta->language[i].offset;
            break;
        }
    }
    if (off == -1) {
        return ErrNoSupportLanguage;
    }
    const char *content;
    err = ipdb_find0(reader, addr, &content);
    if (err != ErrNoErr) {
        return err;
    }

    char **tmp;
    int sum = split(content, '\t', &tmp);
    if (off + reader->meta->fields_length > sum) {
        err = ErrDatabaseError;
    } else {
        ipdb_string_chain *root = malloc(sizeof(ipdb_string_chain));
        ipdb_string_chain *vector = root;
        for (int i = off; i < off + reader->meta->fields_length; ++i) {
            vector->str = malloc(strlen(tmp[i]) + 1);
            strcpy(vector->str, tmp[i]);
            if (i == off + reader->meta->fields_length - 1)break;
            vector->next = malloc(sizeof(ipdb_string_chain));
            vector = vector->next;
            memset(vector, 0, sizeof(ipdb_string_chain));
        }
        *body = root;
        err = ErrNoErr;
    }
    for (int i = 0; i < sum; ++i) {
        free(tmp[i]);
    }
    free(tmp);
    return err;
}

int ipdb_reader_find(ipdb_reader *reader, const char *addr, const char *language, ipdb_string_chain **body) {
    return ipdb_find1(reader, addr, language, body);
}

void ipdb_string_chain_free(ipdb_string_chain **body) {
    ipdb_string_chain *next = *body;
    while (next) {
        ipdb_string_chain *temp = next->next;
        free(next);
        next = temp;
    }
    *body = 0;
}