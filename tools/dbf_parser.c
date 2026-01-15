#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/utils.h"
#include "../src/maths.h"
#include "../src/country.h"

#pragma pack(push,1)

typedef struct
{
    u8 version; // 0x03 dBASE III (typical)
    u8 yy, mm, dd;
    u32 record_count;
    u16 header_size;
    u16 record_size;
    u8 reserved1[18];
    u8 reserved2[2];
} dbf_header_t;

typedef struct
{
    char name_raw[11]; // not guaranteed null-terminated
    char type; // 'C','N','F','L','D','M',...
    u32 field_offset; // from start of record (after deleted flag)
    u8 field_length;
    u8 decimal_count;
    u8 reserved[14];
} dbf_field_t;

#pragma pack(pop)

typedef struct
{
    char name[12]; // null-terminated
    char type; // as above
    u32 offset; // offset within record (after deleted flag)
    u8 length;
    u8 decimals;
} dbf_column_t;

static inline void usage(const char* name)
{
    fprintf(stderr, "Usage: %s --dbf <dbf file>\n", name);
}

static void parse_dbf_file(const u8* dbf_file_buffer)
{
    dbf_header_t* dbf_header = (dbf_header_t*)dbf_file_buffer;
    const u8* dbf_buffer = dbf_file_buffer + sizeof(dbf_header_t);

    dbf_column_t dbf_columns[256] = { 0 };
    u32 dbf_column_count = 0;

    const u8* dbf_fields = dbf_buffer;

    for (;;)
    {
        // NOTE: End of field array.
        if (*dbf_fields == 0x0D)
        {
            break;
        }

        dbf_field_t* dbf_field = (dbf_field_t*)dbf_fields;
        dbf_column_t* dbf_column = dbf_columns + dbf_column_count++;

        memcpy(dbf_column->name, dbf_field->name_raw, 11);
        dbf_column->name[11] = 0;

        dbf_column->type = dbf_field->type;
        dbf_column->offset = dbf_field->field_offset;
        dbf_column->length = dbf_field->field_length;
        dbf_column->decimals = dbf_field->decimal_count;

        if (dbf_column_count >= array_count(dbf_columns))
        {
            assert(!"Too many fields");
        }
        
        dbf_fields += sizeof(dbf_field_t);
    }

    u32 name_index = (u32)-1;
    for(u32 index = 0; index < dbf_column_count; ++index)
    {
        if (!strcmp(dbf_columns[index].name, "NAME"))
        {
            name_index = index;
        }
    }

    if (name_index == (u32)-1)
    {
        assert(!"Column index could not find.");
    }

    const u8* dbf_records = dbf_file_buffer + dbf_header->header_size;
    const dbf_column_t* dbf_name_column = dbf_columns + name_index;

    printf("static country_name_t global_shape_country_names[] = \n{\n");
    
    for (u32 record_index = 0; record_index < dbf_header->record_count; dbf_records += dbf_header->record_size, ++record_index)
    {
        const u8* dbf_record = dbf_records;

        // NOTE: According to standard each record should start with deletion flag
        // but for some reason our dbf file do not use them.
        if (*dbf_record == '*')
        {
            continue;
        }

        u8 name_buffer[256] = { 0 };
        // NOTE: We don't need +1 because our data do not use deletion flag as mentioned above.
        memcpy(name_buffer, dbf_record + /* + 1 + */ dbf_name_column->offset, dbf_name_column->length);
        name_buffer[dbf_name_column->length] = 0;
        printf("    { \"%s\", %zu },\n", name_buffer, strlen((const char*)name_buffer));
    }
    printf("\n};\n");
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        usage(argv[0]);
        return 1;
    }

    const char* dbf_file_opt = argv[1];
    const char* dbf_file_name = argv[2];
    
    if (strcmp("--dbf", dbf_file_opt))
    {
        fprintf(stderr, "Expected '--dbf' found: '%s'\n", dbf_file_opt);
        return 1;
    }

    FILE* dbf_file = fopen(dbf_file_name, "rb");

    if (!dbf_file)
    {
        fprintf(stderr, "Could not open the dbf file: %s\n", dbf_file_name);
        return 1;
    }

    fseek(dbf_file, 0, SEEK_END);
    size_t dbf_file_size = ftell(dbf_file);
    fseek(dbf_file, 0, SEEK_SET);

    u8* dbf_file_buffer = malloc(dbf_file_size + 1);

    if (!dbf_file_buffer)
    {
        fprintf(stderr, "Could not allocate the dbf file buffer\n");
        return 1;
    }

    dbf_file_buffer[dbf_file_size] = '\0';

    size_t read_size = fread(dbf_file_buffer, dbf_file_size, 1, dbf_file);

    if (read_size != 1)
    {
        fprintf(stderr, "Could not read the dbf file: %s\n", dbf_file_name);
        fclose(dbf_file);
        free(dbf_file_buffer);
        return 1;
    }

    parse_dbf_file(dbf_file_buffer);

    free(dbf_file_buffer);
    fclose(dbf_file);

    return 0;
}
