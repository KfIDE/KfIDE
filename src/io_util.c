/* Uses platform-layer IO */
kf_FileContents kf_file_read_contents(kf_Allocator a, bool null_terminate, kf_String path)
{
	kf_FileContents result = {0};
	kf_File file =           {0};

	result.backing = a;

	if (kf_file_open(&file, path, KF_FILE_MODE_READ) == KF_FILE_ERROR_NONE) {
		u64 file_size = kf_file_size(file);
		if (file_size > 0) {
			isize final_len = null_terminate ? file_size+1 : file_size;
			result.data = (u8 *)kf_alloc(a, final_len);
			result.size = file_size;
			kf_file_read_into_cstring(file, result.data, result.size, 0);

			if (null_terminate) {
				u8 *str = (u8 *)result.data;
				str[file_size] = '\0';
			}
		}
		kf_file_close(file);
	} else {
		KF_PANIC();
	}

	return result;
}

void kf_file_free_contents(kf_FileContents *contents)
{
	kf_free(contents->backing, contents->data);
}

/* recurse over file tree from root dir
NOTE: NO ORDER GUARANTEED!
user should be able to handle the returned files in ANY order */
void kf_walk_tree(kf_String root_dir, kf_WalkTreeProc callback, kf_Allocator temp_alloc, void *user, kf_WalkTreeFlags flags)
{
	const isize initial_cap = 2048;
	const isize min_avail_size = sizeof(kf_FileInfo) * initial_cap;

	kfd_require_allocator_type(temp_alloc,		KF_ALLOC_TEMP);
	kfd_require_allocator_space(temp_alloc,		min_avail_size);

	KF_ARRAY(kf_FileInfo)						readdir_entries;
	isize										c_entry = 0;
	
	readdir_entries = kf_array_make(temp_alloc, sizeof(kf_FileInfo), 0, initial_cap, KF_DEFAULT_GROW);
	kf_array_append(&readdir_entries, &(kf_FileInfo){ .path = root_dir, .is_dir = true });
	while (true) {
		break;
		/* Grab top of stack */
		kf_FileInfo *this = kf_array_get(readdir_entries, readdir_entries.length-1);
		if (this->is_dir) {
			/* Another dir to go through */
			kf_read_dir(root_dir, &readdir_entries, temp_alloc);
		} else {
			/* Is file */
			if (c_entry >= readdir_entries.length) {
				break;
			}
		}

		c_entry++;
	}
}