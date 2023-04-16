#include <string.h>

/* Parses through .csv data */
#define TR_DELIM ";;"
typedef enum { TranslationLoadPhase_HEADER, TranslationLoadPhase_BODY } TranslationLoadPhase;
void kf_load_translations_from_csv_string(gbAllocator alloc, TranslationRecord *record, u8 *data, isize length)
{
	isize i;
	isize num_header_entries;
	isize body_index, lang_offset;
	u8 this_char;
	u8 this_char_buf[2]; /* always contains this_char and then NULL, so that we can pass it to gb_string* procs */
	gbString buf;
	gbString permanent_buf; /* we 'leak' mem allocs into this var, but they are copied into the TrRecord anyway so it's fine */
	TranslationLoadPhase phase;

	GB_ASSERT(record != NULL && data != NULL);

	if (length < 0) {
		length = strlen(data);
	}

	buf = gb_string_make_reserve(alloc, 4096); /* gb_string_free() at end */

	phase = TranslationLoadPhase_HEADER;
	num_header_entries = 0;
	body_index = 0;
	lang_offset = 0;

	this_char_buf[1] = '\0';
	for (i = 0; i < length; i++) {
		this_char = data[i];
		this_char_buf[0] = this_char;
		gb_string_appendc(buf, (const char *)this_char_buf); /* [0] = this_char, [1] = NULL */
		isize buflen = gb_string_length(buf);

		/*printf("%s\n", buf);*/
		if (phase == TranslationLoadPhase_HEADER) {
			if (buflen >= 2) /* Hit ";;", so we must have gotten an entry */ {
				bool ends_in_delim = strncmp(&buf[buflen - 2], TR_DELIM, 2) == 0;
				bool ends_in_newline = strncmp(&buf[buflen - 1], "\n", 1) == 0;

				if (ends_in_delim || ends_in_newline) {
					/* Perma-allocate the string and add it to langs part of record */
					if (ends_in_delim) {
						permanent_buf = gb_string_make_length(alloc, buf, buflen - 2);
					} else {
						permanent_buf = gb_string_make_length(alloc, buf, buflen - 1);
					}

					record->lang_keys[record->lang_head++] = permanent_buf;
					gb_string_clear(buf);
				}

				if (ends_in_newline) {
					phase = TranslationLoadPhase_BODY;
				}
			}
		} else if (phase == TranslationLoadPhase_BODY) {
			if (buflen >= 2) {
				bool ends_in_delim = strncmp(&buf[buflen - 2], TR_DELIM, 2) == 0;
				bool ends_in_newline = strncmp(&buf[buflen - 1], "\n", 1) == 0;

				if (ends_in_delim || ends_in_newline) {
					if (ends_in_delim) {
						permanent_buf = gb_string_make_length(alloc, buf, buflen - 2);
					} else {
						permanent_buf = gb_string_make_length(alloc, buf, buflen - 1);
					}

					isize index = (record->lang_head * body_index) + lang_offset++;
					record->values[index] = permanent_buf;
					record->num_entries++;

					if (ends_in_newline) {
						lang_offset = 0;
						body_index++;
					}

					gb_string_clear(buf);
				}
			}
		}
	}

	gb_string_free(buf);
}

void kf_tr_select_lang(TranslationRecord *record, u8 *selection)
{
	isize i;

	for (i = 0; i < record->lang_head; i++) {
		if (strcmp(record->lang_keys[i], selection) == 0) {
			record->selected_lang = selection;
			record->selected_lang_offset = i;
			return;
		}
	}
	printf("Selected invalid language: %s", selection);
	GB_PANIC("");
}

u8 *kf_tr_query(TranslationRecord *record, isize index)
{
	isize calculated_index;

	calculated_index = (record->lang_head * index) + record->selected_lang_offset;
	GB_ASSERT(calculated_index > -1 && calculated_index < record->num_entries);
	return record->values[calculated_index];
}

void kf_tr_print(TranslationRecord *record)
{
	isize i;

	for (i = 0; i < record->lang_head; i++) {
		printf("%s", record->lang_keys[i]);
	}

	for (i = 0; i < record->num_entries; i++) {
		printf("%s", record->values[i]);
	}
}