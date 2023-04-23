#include "kf.h" /* NOTE(EimaMei): vscode stfu */


/* kf_system_font_paths should be defined in the platform_* file */
static const u8 KF_TTF_EXT[] =  ".ttf";
static int _query_fonts_callback(kf_Allocator heap_alloc, kf_Allocator temp_alloc, kf_String this_path, void *user)
{
    KF_ARRAY(kf_String) *query_out = user; /* county out */

    if (kf_string_ends_with_cstring(this_path, KF_TTF_EXT, strlen(KF_TTF_EXT))) {
        kf_array_append(query_out, &this_path);
    }
    return 0;
}

void kf_query_system_fonts(kf_Allocator temp_alloc, KF_ARRAY(kf_String) *out)
{
	isize i;
	kf_String root_as_kf_string, joined;
    isize cur_ent, max_ents;
    isize this_strlen;

	for (i = 0; i < KF_NUM_SYSTEM_FONT_PATHS; i++) {
        /* Check if "" */
        this_strlen = strlen(kf_system_font_paths[i]);
        if (this_strlen <= 0) {
            break;
        }

		root_as_kf_string = kf_string_set_from_cstring_len(kf_system_font_paths[i], this_strlen); /* e.g. '/usr/share/fonts' */

		/* Recursive read dir */
        kfd_printf("Querying fonts in %s", kf_system_font_paths[i]);
        kf_walk_tree(root_as_kf_string, _query_fonts_callback, temp_alloc, (void *)out, KF_WALK_TREE_FILES_ONLY);
	}
}

void kfd_print_system_fonts(KF_ARRAY(kf_String) query_result)
{
#ifdef KF_DEBUG
    isize i;
    for (i = 0; i < query_result.length; i++) {
        kf_String *s = kf_array_get(query_result, i);
        kfd_printf("QUERIED FONT: %s", s->ptr);
    }
#endif
}