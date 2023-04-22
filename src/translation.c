#include <string.h>

/* Parses through .csv data */
#define TR_DELIM ";;"
typedef enum { TranslationLoadPhase_HEADER, TranslationLoadPhase_BODY } TranslationLoadPhase;
void kf_load_translations_from_csv_buffer(kf_Allocator alloc, kf_TranslationRecord *record, u8 *data, isize length)
{
	kf_String permanent_buf; /* we 'leak' mem allocs into this var, but they are copied into the TrRecord anyway so it's fine */

	KF_ASSERT(record != NULL && data != NULL);

	if (length < 0) {
		length = strlen(data);
	}

	kf_String buf_string = kf_string_make_length_capacity(alloc, 0, 4096); /* gb_string_free() at end */
	u8 *buf = buf_string.cstr;

	TranslationLoadPhase phase = TranslationLoadPhase_HEADER;
	isize num_header_entries = 0;
	isize body_index = 0;
	isize lang_offset = 0;

	isize i;
	u8 this_char;
	for (i = 0; i < length; i++) {
		this_char = data[i];
		kf_append_rune_to_string(&buf_string, (Rune)this_char);
		isize buflen = buf_string.length;

		/*printf("%s\n", buf);*/
		if (phase == TranslationLoadPhase_HEADER) {
			if (buflen >= 2) /* Hit ";;", so we must have gotten an entry */ {
				bool ends_in_delim = strncmp(&buf[buflen - 2], TR_DELIM, 2) == 0;
				bool ends_in_newline = strncmp(&buf[buflen - 1], "\n", 1) == 0;

				if (ends_in_delim || ends_in_newline) {
					/* Perma-allocate the string and add it to langs part of record */
					if (ends_in_delim) {
						permanent_buf = kf_string_copy_from_cstring_length(alloc, buf, buflen - 2);
					} else {
						permanent_buf = kf_string_copy_from_cstring_length(alloc, buf, buflen - 1);
					}

					record->lang_keys[record->lang_head++] = permanent_buf.cstr;
					kf_string_clear(&buf_string);
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
						permanent_buf = kf_string_copy_from_cstring_length(alloc, buf, buflen - 2);
					} else {
						permanent_buf = kf_string_copy_from_cstring_length(alloc, buf, buflen - 1);
					}

					isize index = (record->lang_head * body_index) + lang_offset++;
					record->values[index] = permanent_buf.cstr;
					record->num_entries++;

					if (ends_in_newline) {
						lang_offset = 0;
						body_index++;
					}

					kf_string_clear(&buf_string);
				}
			}
		}
	}
}

void kf_tr_select_lang(kf_TranslationRecord *record, u8 *selection)
{
	isize i;

	for (i = 0; i < record->lang_head; i++) {
		if (strcmp(record->lang_keys[i], selection) == 0) {
			record->selected_lang = selection;
			record->selected_lang_offset = i;
			return;
		}
	}
	printf("Selected invalid language: %s\n", selection);
	KF_PANIC("");
}

u8 *kf_tr_query(kf_TranslationRecord *record, isize index)
{
	isize calculated_index;

	calculated_index = (record->lang_head * index) + record->selected_lang_offset;
	KF_ASSERT(calculated_index > -1 && calculated_index < record->num_entries);
	return record->values[calculated_index];
}

void kfd_print_translations(kf_TranslationRecord record)
{
#ifdef KF_DEBUG
	isize i;

	for (i = 0; i < record.lang_head; i++) {
		printf("%s\n", record.lang_keys[i]);
	}

	for (i = 0; i < record.num_entries; i++) {
		printf("%s\n", record.values[i]);
	}
#endif
}