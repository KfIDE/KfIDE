isize kf_lookup_internal_glyph_index_by_rune(kf_Font *font, Rune r)
{
    isize internal_index = -1;
    isize i;
    for (i = 0; i < gb_array_count(font->runes); i++) {
        if (font->runes[i] == r) {
            internal_index = i;
            break;
        }
    }

    if (internal_index < 0) {
        GB_PANIC("kf_lookup_internal_glyph_index_by_rune(): rune not present in given font.");
    }
    return internal_index;
}

/* kf_system_font_paths should be defined in the platform_* file */
void kf_query_system_fonts(gbAllocator str_alloc, gbAllocator temp_alloc, gbArray(gbString) out)
{
	isize num_system_fonts;
	isize i;
	gbString root_as_gb, joined;

	num_system_fonts = sizeof(system_fonts) / sizeof(kf_system_font_paths[0]);
	for (i = 0; i < num_system_fonts; i++) {
		root_as_gb = gb_string_make(str_alloc, kf_system_font_paths[i]); /* e.g. '/usr/share/fonts' */

		/* Recursive read dir */
		isize depth = 0;
		bool current_is_dir = true;
		gbString current_path = ;
		while (true) {
			// gbArray(kf_FileInfo) entries;
			// gb_array_init_reserve(entries, temp_alloc, 256);
			// kf_read_dir(path, entries, temp_alloc);

			if (depth == 0) { /*  */

			}
		}

		/* joined = kf_join_paths(root_as_gb, ); */
	}
}