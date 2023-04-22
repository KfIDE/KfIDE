/* utf8 string deocde */
static const u8 kf__utf8_first[256] = { /* Copied from gb.h */
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, /* 0x00-0x0F */
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, /* 0x10-0x1F */
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, /* 0x20-0x2F */
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, /* 0x30-0x3F */
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, /* 0x40-0x4F */
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, /* 0x50-0x5F */
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, /* 0x60-0x6F */
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, /* 0x70-0x7F */
	0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, /* 0x80-0x8F */
	0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, /* 0x90-0x9F */
	0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, /* 0xA0-0xAF */
	0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, /* 0xB0-0xBF */
	0xf1, 0xf1, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xC0-0xCF */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 0xD0-0xDF */
	0x13, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x23, 0x03, 0x03, /* 0xE0-0xEF */
	0x34, 0x04, 0x04, 0x04, 0x44, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, /* 0xF0-0xFF */
};

static const kf_Utf8AcceptRange kf__utf8_accept_ranges[] = { /* Copied from gb.h */
	{0x80, 0xbf},
	{0xa0, 0xbf},
	{0x80, 0x9f},
	{0x90, 0xbf},
	{0x80, 0x8f},
};

isize kf_decode_utf8_single(u8 *str, isize str_len, rune *codepoint_out) /* Copied from gb.h */
{
	isize width = 0;
	rune codepoint = KF_RUNE_INVALID;

	if (str_len > 0) {
		u8 s0 = str[0];
		u8 x = kf__utf8_first[s0], sz;
		u8 b1, b2, b3;
		kf_Utf8AcceptRange accept;
		if (x >= 0xf0) {
			rune mask = ((rune)x << 31) >> 31;
			codepoint = ((rune)s0 & (~mask)) | (KF_RUNE_INVALID & mask);
			width = 1;
			goto end;
		}
		if (s0 < 0x80) {
			codepoint = s0;
			width = 1;
			goto end;
		}

		sz = x&7;
		accept = kf__utf8_accept_ranges[x>>4];
		if (str_len < sizeof(sz))
			goto invalid_codepoint;

		b1 = str[1];
		if (b1 < accept.lo || accept.hi < b1)
			goto invalid_codepoint;

		if (sz == 2) {
			codepoint = ((rune)s0&0x1f)<<6 | ((rune)b1&0x3f);
			width = 2;
			goto end;
		}

		b2 = str[2];
		if (!KF_IS_BETWEEN(b2, 0x80, 0xbf))
			goto invalid_codepoint;

		if (sz == 3) {
			codepoint = ((rune)s0&0x1f)<<12 | ((rune)b1&0x3f)<<6 | ((rune)b2&0x3f);
			width = 3;
			goto end;
		}

		b3 = str[3];
		if (!KF_IS_BETWEEN(b3, 0x80, 0xbf))
			goto invalid_codepoint;

		codepoint = ((rune)s0&0x07)<<18 | ((rune)b1&0x3f)<<12 | ((rune)b2&0x3f)<<6 | ((rune)b3&0x3f);
		width = 4;
		goto end;

	invalid_codepoint:
		codepoint = KF_RUNE_INVALID;
		width = 1;
	}

end:
	if (codepoint_out) *codepoint_out = codepoint;
	return width;
}

void kf_decode_utf8_string_to_rune_array(kf_String str, KF_ARRAY(rune) *rune_array)
{
	/* Assumes rune_array already initialized */
	isize this_char, string_length;
	rune r = KF_RUNE_INVALID;

	string_length = str.length;
	isize rune_count = 0;
	for (this_char = 0; this_char < string_length;) {
		this_char += kf_decode_utf8_single(&str.ptr[this_char], (string_length - this_char), &r);
		KF_ASSERT(r != KF_RUNE_INVALID);
		kf_array_append(rune_array, &r);
		/*kfd_printf("IM DECODE %d ::: R DECODE ARR %d ::: OTHER GET %d", r, KF_ARRAY_GET(*rune_array, rune_count, rune), *((rune *)(&rune_array->start[4 * rune_count])));*/

		rune_count++;
	}
}

/* rune encoding */
isize kf_write_rune_as_utf8(u8 buf[4], rune r) {
	u32 i = (u32)r;
	u8 mask = 0x3f;
	if (i <= (1<<7)-1) {
		buf[0] = (u8)r;
		return 1;
	}
	if (i <= (1<<11)-1) {
		buf[0] = 0xc0 | (u8)(r>>6);
		buf[1] = 0x80 | ((u8)(r)&mask);
		return 2;
	}

	/* Invalid or Surrogate range */
	if (i > KF_RUNE_MAX ||
	    KF_IS_BETWEEN(i, 0xd800, 0xdfff)) {
		r = KF_RUNE_INVALID;

		buf[0] = 0xe0 | (u8)(r>>12);
		buf[1] = 0x80 | ((u8)(r>>6)&mask);
		buf[2] = 0x80 | ((u8)(r)&mask);
		return 3;
	}

	if (i <= (1<<16)-1) {
		buf[0] = 0xe0 | (u8)(r>>12);
		buf[1] = 0x80 | ((u8)(r>>6)&mask);
		buf[2] = 0x80 | ((u8)(r)&mask);
		return 3;
	}

	buf[0] = 0xf0 | (u8)(r>>18);
	buf[1] = 0x80 | ((u8)(r>>12)&mask);
	buf[2] = 0x80 | ((u8)(r>>6)&mask);
	buf[3] = 0x80 | ((u8)(r)&mask);
	return 4;
}

/* Encodes rune to utf8 and appends that u8[] to the input string */
void kf_append_rune_to_string(kf_String *str, rune r)
{
    if (r >= 0) {
		u8 buf[8] = {0};
		isize len = kf_write_rune_as_utf8(buf, r);
		kf_string_append_cstring_len(str, buf, len);
	}
}

/* basically this just returns a sub-string up to the last '/' index, not including the '/' itself */
kf_String kf_path_parentdir(kf_String path, kf_Allocator new_path_alloc)
{
	isize last_slash = -1;
	isize i;
	for (i = 0; i < path.length; i++) {
		if (path.ptr[i] == kf_path_separator) {
			last_slash = i;
		}
	}

	if (last_slash == -1) {
		KF_PANIC("Invalid path given to kf_path_parentdir()");
	}

	/* Re-construct path as a kf_String but with shortened length; no null-termination */
	kf_String out = kf_string_set_from_cstring_len(path.ptr, last_slash);
	return out;
}

kf_String kf_join_paths(kf_Allocator join_alloc, kf_String base, kf_String add)
{
	isize base_strlen, add_strlen, alloc_size;
	kf_String out;

	base_strlen = base.length;
	add_strlen = add.length;
	alloc_size = base_strlen + add_strlen + 1 + 1; /* +1 for NULL, +1 for kf_path_separator */
	out = kf_string_make(join_alloc, alloc_size, alloc_size, KF_DEFAULT_STRING_GROW);

	/* NOTE: out contains a fresh/unique cstr */
	memcpy(out.ptr, base.ptr, base_strlen);
	out.ptr[base_strlen] = kf_path_separator; /* Add in '/' (or '\' on win33) to conjunction point of the 2 strings */
	memcpy(&out.ptr[base_strlen + 1], add.ptr, add_strlen);
	out.ptr[alloc_size - 1] = '\0';

	return out;
}

bool kf_string_ends_with(kf_String s, kf_String cmp)
{
	return strncmp(&s.ptr[s.length - cmp.length], cmp.ptr, cmp.length) == 0;
}

bool kf_string_ends_with_cstring(kf_String s, u8 *cmp, isize cmp_strlen)
{
	return strncmp(&s.ptr[s.length - cmp_strlen], cmp, cmp_strlen) == 0;
}