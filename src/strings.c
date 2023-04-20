// utf8 string decode
void kf_decode_utf8_string_to_rune_array(gbString str, gbArray(Rune) rune_array)
{
	// Assumes rune_array already initialized
	isize this_char, string_length;
	Rune r = GB_RUNE_INVALID;

	string_length = gb_string_length(str);
	for (this_char = 0; this_char < string_length;) {
		this_char += gb_utf8_decode(&str[this_char], (string_length - this_char), &r);
		GB_ASSERT(r != GB_RUNE_INVALID);
		gb_array_append(rune_array, r);
	}
}

/* basically this just returns a sub-string up to the last '/' index, not including the '/' itself */
gbString kf_path_parentdir(gbString path, gbAllocator new_path_alloc)
{
	isize last_slash = -1;
	isize i;
	for (i = 0; i < gb_string_length(path); i++) {
		if (path[i] == GB_PATH_SEPARATOR) {
			last_slash = i;
		}
	}

	if (last_slash == -1) {
		GB_PANIC("Invalid path given to kf_path_parentdir()");
	}

	gbString out = gb_string_make_reserve(new_path_alloc, last_slash + 1);
	out[last_slash] = '\0';
	GB_STRING_HEADER(out)->length = last_slash;
	return path;
}

gbString kf_path_join(gbAllocator join_alloc, gbString base, gbString add)
{
	isize base_strlen, add_strlen, alloc_size;
	gbString out;

	base_strlen = gb_string_length(base);
	add_strlen = gb_string_length(add);
	alloc_size = base_strlen + add_strlen + 1 + 1; /* +1 for NULL, +1 for GB_PATH_SEPARATOR */
	out = gb_string_make_reserve(join_alloc, alloc_size);

	gb_memcopy(out, base, base_strlen);
	out[base_strlen] = GB_PATH_SEPARATOR; /* Add in '/' (or '\' on win33) to conjunction point of the 2 strings */
	gb_memcopy(&out[base_strlen + 1], add, add_strlen);
	out[alloc_size - 1] = '\0';

	return out;
}