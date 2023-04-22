/*
if align is 16
if n is     42
then...

16     = ...01_0000
n      = ...10_1010
n | 15 = ...10_1111 (15 = align-1)
n + 1  = ...11_0000
*/
isize kf_align_ceil(isize n) /* Rounds integer to nearest aligned int (ceil). If already multiple of align, just return the input num. */
{
    isize out;

    if (n % KF_DEFAULT_MEMORY_ALIGNMENT == 0) {
        return n;
    }

    out = n;
    out |= KF_DEFAULT_MEMORY_ALIGNMENT - 1;
    out++;
    return out;
}

void *kf_alloc(kf_Allocator alloc, isize num_bytes)
{
    return ((kf_AllocatorProc)alloc.allocate) (alloc, kf_AllocationType_ALLOC, NULL, num_bytes, -1);
}

void *kf_resize(kf_Allocator alloc, void *ptr, isize num_bytes, isize old_size)
{
    if (old_size < 0) {
        old_size = num_bytes;
    }
    return ((kf_AllocatorProc)alloc.allocate) (alloc, kf_AllocationType_RESIZE, ptr, num_bytes, old_size);
}

void kf_free(kf_Allocator alloc, void *ptr)
{
    ((kf_AllocatorProc)alloc.allocate) (alloc, kf_AllocationType_FREE, ptr, -1, -1);
}

void kf_free_all(kf_Allocator alloc)
{
    ((kf_AllocatorProc)alloc.allocate) (alloc, kf_AllocationType_FREE_ALL, NULL, -1, -1);
}

isize kf_query_avail_space(kf_Allocator alloc)
{
    return ((kf_AllocatorProc)alloc.allocate) (alloc, kf_AllocationType_QUERY_AVAIL_SPACE, NULL, -1, -1);
}

void kfd_require_allocator_type(kf_Allocator alloc, kf_AllocatorType type_required)
{
#ifdef KF_DEBUG
    if (alloc.type != type_required) {
        KF_PANIC("Allocator did not meet type requirement!");
    }
#endif
}

void kfd_require_allocator_space(kf_Allocator alloc, isize at_least)
{
#ifdef KF_DEBUG
    isize avail = kf_query_avail_space(alloc);
    if (avail < 0) {
        kfd_printf("WARN: Tried to force space requirement on allocator but it did not implement a query.");
    } else if (avail < at_least) {
        KF_PANIC("kfd_require_allocator_space() check failed!");
    }
#endif
}


/* ARRAYS */
kf_Array kf_array_make_capacity(kf_Allocator alloc, isize type_width, isize initial_capacity)
{
    return kf_array_make_length_capacity_grow(alloc, type_width, 0, initial_capacity, KF_DEFAULT_GROW);
}

kf_Array kf_array_make_length_capacity(kf_Allocator alloc, isize type_width, isize initial_length, isize initial_capacity)
{
    return kf_array_make_length_capacity_grow(alloc, type_width, initial_length, initial_capacity, KF_DEFAULT_GROW);
}

kf_Array kf_array_make_length_capacity_grow(kf_Allocator alloc, isize type_width, isize initial_length, isize initial_capacity, isize grow)
{
    kf_Array out;
    out.start = (u8 *)kf_alloc(alloc, type_width * initial_capacity);
    out.backing = alloc;
    out.type_width = type_width;
    out.length = initial_length;
    out.capacity = initial_capacity;
    out.grow = grow;
    return out;
}

kf_Array kf_array_from_ptr(kf_Allocator alloc, void *ptr, isize type_width, isize num_elements)
{
    return kf_array_from_ptr_grow(alloc, ptr, type_width, num_elements, KF_DEFAULT_GROW);
}

kf_Array kf_array_from_ptr_grow(kf_Allocator alloc, void *ptr, isize type_width, isize num_elements, isize grow)
{
    kf_Array out;
    out.start = (u8 *)ptr;
    out.backing = alloc;
    out.type_width = type_width;
    out.length = num_elements;
    out.capacity = num_elements;
    out.grow = grow;
    return out;
}

void *kf_array_get(kf_Array array, isize index)
{
    KF_ASSERT(index >= 0 && index < array.length);

    return &array.start[index * array.type_width];
}

void *kf_array_get_type(kf_Array array, isize index, isize type_width)
{
    KF_ASSERT(type_width > 0 && type_width == array.type_width);

    return kf_array_get(array, index);
}

void kf_array_set(kf_Array *array, isize index, u8 *data)
{
    KF_ASSERT(index >= 0 && index < array->length);
    memcpy(&array->start[index * array->type_width], data, array->type_width);
}

isize kf_array_next_capacity(kf_Array array)
{
    return array.capacity + array.grow;
}

void kf_array_resize(kf_Array *array, isize new_capacity)
{
    array->start = (u8 *)kf_resize(array->backing, array->start, new_capacity * array->type_width, array->capacity * array->type_width);
    array->capacity = new_capacity;
}

void kf_array_append(kf_Array *array, void *new_element)
{
    kf_array_append_multi(array, new_element, 1);
}

void kf_array_append_multi(kf_Array *array, void *new_elements, isize num)
{
    if (kf_array_is_frozen(*array)) {
        KF_PANIC("Cannot append to frozen array!");
    }

    isize new_length = array->length + num;
    if (new_length > array->capacity) {
        kf_array_resize(array, new_length);
    }

    memcpy(&array->start[array->length * array->type_width], new_elements, array->type_width * num);
    array->length = new_length;
}

void kf_array_pop(kf_Array *array)
{
    if (array->length > 0) {
        array->length--;
    } else {
        kfd_printf("WARN: tried to kf_array_pop() but the array has no elements!");
    }
}

void kf_array_clear(kf_Array *array)
{
    array->length = 0;
}

void kf_array_freeze(kf_Array *array)
{
    array->backing.allocate = (kf_AllocatorProc)NULL;
}

void kf_array_unfreeze(kf_Array *array, kf_Allocator alloc)
{
    array->backing = alloc;
}

bool kf_array_is_frozen(kf_Array array)
{
    return (bool)(array.backing.allocate == NULL);
}

void kf_array_free(kf_Array array)
{
    kf_free(array.backing, array.start);
}

/* STRINGS */
kf_String kf_string_set_from_cstring(u8 *cstr)
{
    isize length = strlen(cstr);
    return kf_string_set_from_cstring_length_capacity_grow(cstr, length, length, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_set_from_cstring_length(u8 *cstr, isize length)
{
    return kf_string_set_from_cstring_length_capacity_grow(cstr, length, length, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_set_from_cstring_length_capacity(u8 *cstr, isize length, isize capacity)
{
    return kf_string_set_from_cstring_length_capacity_grow(cstr, length, capacity, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_set_from_cstring_length_capacity_grow(u8 *cstr, isize length, isize capacity, isize grow)
{
    if (cstr == NULL) {
        kfd_printf("WARN: Creating string from cstring (no copy) but given cstring is NULL.");
    }

    kf_String out;
    out.cstr = cstr;
    out.backing = (kf_Allocator){0};
    out.length = length;
    out.capacity = length;
    out.grow = grow;
    return out;
}

kf_String kf_string_copy_from_cstring(kf_Allocator alloc, u8 *cstr)
{
    isize length = strlen(cstr);
    return kf_string_copy_from_cstring_length_capacity_grow(alloc, cstr, length, length, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_copy_from_cstring_length(kf_Allocator alloc, u8 *cstr, isize length)
{
    return kf_string_copy_from_cstring_length_capacity_grow(alloc, cstr, length, length, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_copy_from_cstring_length_capacity(kf_Allocator alloc, u8 *cstr, isize length, isize capacity)
{
    return kf_string_copy_from_cstring_length_capacity_grow(alloc, cstr, length, capacity, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_copy_from_cstring_length_capacity_grow(kf_Allocator alloc, u8 *cstr, isize length, isize capacity, isize grow)
{
    KF_ASSERT(alloc.allocate != NULL && cstr != NULL && length > -1 && capacity > -1 && grow > 0);

    kf_String out;
    out.cstr = (u8 *)kf_alloc(alloc, capacity);
    out.length = length;
    out.capacity = capacity;
    out.grow = grow;

    memcpy(out.cstr, cstr, length);
    return out;
}

kf_String kf_string_copy_from_string(kf_Allocator alloc, kf_String to_clone)
{
    return kf_string_copy_from_string_length_capacity_grow(alloc, to_clone, to_clone.length, to_clone.capacity, to_clone.grow);
}

kf_String kf_string_copy_from_string_length(kf_Allocator alloc, kf_String to_clone, isize substr_length)
{
    return kf_string_copy_from_string_length_capacity_grow(alloc, to_clone, substr_length, to_clone.capacity, to_clone.grow);
}

kf_String kf_string_copy_from_string_length_capacity(kf_Allocator alloc, kf_String to_clone, isize substr_length, isize capacity)
{
    return kf_string_copy_from_string_length_capacity_grow(alloc, to_clone, substr_length, capacity, to_clone.grow);
}

kf_String kf_string_copy_from_string_length_capacity_grow(kf_Allocator alloc, kf_String to_clone, isize substr_length, isize capacity, isize grow)
{
    KF_ASSERT(substr_length <= capacity && to_clone.cstr != NULL && substr_length > -1 && capacity > -1 && grow > 0);

    kf_String out = to_clone;
    out.cstr = (u8 *)kf_alloc(alloc, capacity);
    memcpy(out.cstr, to_clone.cstr, substr_length);

    out.backing = alloc;
    out.length = substr_length;
    out.capacity = capacity;
    out.grow = grow;
    return out;
}

kf_String kf_string_make_length(kf_Allocator alloc, isize length)
{
    return kf_string_make_length_capacity_grow(alloc, length, length, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_make_length_zeroed(kf_Allocator alloc, isize length)
{
    return kf_string_make_length_capacity_grow_zeroed(alloc, length, length, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_make_length_capacity(kf_Allocator alloc, isize length, isize capacity)
{
    return kf_string_make_length_capacity_grow(alloc, length, capacity, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_make_length_capacity_zeroed(kf_Allocator alloc, isize length, isize capacity)
{
    return kf_string_make_length_capacity_grow_zeroed(alloc, length, capacity, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_make_length_capacity_grow(kf_Allocator alloc, isize length, isize capacity, isize grow)
{
    KF_ASSERT(alloc.allocate != (kf_AllocatorProc)NULL && length > -1 && capacity > -1 && grow > 0);

    kf_String out;
    out.backing = alloc;
    out.cstr = (u8 *)kf_alloc(alloc, capacity);
    out.length = length;
    out.capacity = capacity;
    out.grow = grow;

    return out;
}

kf_String kf_string_make_length_capacity_grow_zeroed(kf_Allocator alloc, isize length, isize capacity, isize grow)
{
    kf_String out = kf_string_make_length_capacity_grow(alloc, length, capacity, grow);
    memset(out.cstr, 0, length);
    return out;
}

void kf_string_freeze(kf_String *str)
{
    str->backing.allocate = (kf_AllocatorProc)NULL;
}

void kf_string_unfreeze(kf_String *str, kf_Allocator alloc)
{
    str->backing = alloc;
    if (kf_string_is_frozen(*str)) {
        kfd_printf("WARN: Tried to kf_string_unfreeze() but failed due to provided allocator also being in a frozen state!");
    }
}

bool kf_string_is_frozen(kf_String str)
{
    return (bool)(str.backing.allocate == NULL);
}

isize kf_string_next_capacity(kf_String str)
{
    return str.capacity + str.grow;
}

void kf_string_resize(kf_String *str, isize new_capacity)
{
    str->capacity = new_capacity;
}

void kf_string_clear(kf_String *str)
{
    str->length = 0;
}

void kf_string_free(kf_String str)
{
    if (kf_string_is_frozen(str)) {
        KF_PANIC("Tried to kf_string_free() a frozen string! Unfreeze it first and try again.");
    }
    kf_free(str.backing, str.cstr);
}

void kf_string_safe_set_char(kf_String *str, isize at, u8 new_char)
{
    KF_ASSERT(str != NULL && at >= 0 && at <= str->length && !kf_string_is_frozen(*str));

    str->cstr[at] = new_char;
}

void kf_string_safe_write_cstring_at(kf_String *str, u8 *cstr, isize at)
{
    kf_string_safe_write_cstring_at_length(str, cstr, at, strlen(cstr));
}

void kf_string_safe_write_cstring_at_length(kf_String *str, u8 *cstr, isize at, isize length)
{
    KF_ASSERT(str != NULL && cstr != NULL && at >= 0 && at <= length);

    memcpy(&str->cstr[at], cstr, length);
}

void kf_string_safe_write_string_at(kf_String *str, kf_String other, isize at)
{
    kf_string_safe_write_cstring_at_length(str, other.cstr, at, other.length);
}

void kf_string_safe_write_string_at_length(kf_String *str, kf_String other, isize at, isize length)
{
    kf_string_safe_write_cstring_at_length(str, other.cstr, at, length);
}

void kf_string_append_cstring(kf_String *str, u8 *cstr)
{
    kf_string_append_cstring_length(str, cstr, strlen(cstr));
}

void kf_string_append_cstring_length(kf_String *str, u8 *cstr, isize length)
{
    KF_ASSERT(str != NULL && cstr != NULL);

    isize new_length = str->length + length;
    if (new_length > str->capacity) {
        kf_string_resize(str, new_length);
    }

    memcpy(&str->cstr[str->length], cstr, length);
    str->length = new_length;
}

void kf_string_append_string(kf_String *str, kf_String other)
{
    kf_string_append_cstring_length(str, other.cstr, other.length);
}

void kf_string_append_string_length(kf_String *str, kf_String other, isize slice_other_upto)
{
    kf_string_append_cstring_length(str, other.cstr, slice_other_upto);
}

u8 kf_string_pop(kf_String *str)
{
    return str->cstr[--str->length];
}


/* *** ALLOCATORS*** */
static void _set_alloc_type(kf_Allocator *a, kf_AllocatorType type)
{
#ifdef KF_DEBUG
    a->type = type;
#endif
}

/* HEAP ALLOCATOR */
void *kf_heap_allocator_proc(kf_Allocator alloc, kf_AllocationType type, void *ptr, isize num_bytes, isize old_size)
{
    void *out = NULL;

    isize aligned_bytes = kf_align_ceil(num_bytes);

    switch (type) {
    case kf_AllocationType_ALLOC: {
        out = malloc(aligned_bytes);
    } break;

    case kf_AllocationType_RESIZE: {
        out = realloc(ptr, aligned_bytes);
    } break;

    case kf_AllocationType_FREE: {
        free(ptr);
    } break;

    case kf_AllocationType_FREE_ALL: {
        /* KF_PANIC("Could not FREE_ALL: is heap allocator"); */
    } break;

    case kf_AllocationType_QUERY_AVAIL_SPACE: {
        return -1; /* yeah, we can't really implement this for the heap */
    } break;

    default:
        KF_PANIC();

    }
    
    return out;
}

kf_Allocator kf_heap_allocator(void)
{
    kf_Allocator out;
    out.allocate = kf_heap_allocator_proc;
    out.user = NULL;
    _set_alloc_type(&out, kf_AllocatorType_HEAP);
    return out;
}

/* NOTE(ps4star): temp allocators use a fixed linear buffer
this is kind of like a ring buffer but it doesn't wrap.
e.g. say the buf is 1MB
if 512K is already alloced, and an incoming alloc is for 700K, panic */
void *kf_temp_allocator_proc(kf_Allocator alloc, kf_AllocationType type, void *ptr, isize num_bytes, isize old_size)
{
    void *out = NULL;
    kf_TempAllocatorData *data = (kf_TempAllocatorData *)alloc.user;

    isize aligned_bytes = kf_align_ceil(num_bytes);

    switch (type) {
    case kf_AllocationType_ALLOC: {
        /* Sub-allocate within buffer */
        isize new_pos = data->position + aligned_bytes;
        if (new_pos >= data->size) {
            kfd_printf("<temp> Could not ALLOC: it exceeds max size!\nBuffer size: %d K\nMax avail: %d B\nYou wanted: %d B", data->size / KF_KILO(1), (data->size - data->position), aligned_bytes);
            KF_PANIC();
        }

        out = &data->start[data->position];
        data->position = new_pos;
    } break;

    case kf_AllocationType_RESIZE: {
        /* This is very inefficient and bad
        You have to call the above _ALLOC code and then memcpy into the new block */
        out = kf_alloc(alloc, aligned_bytes);
        memcpy(out, ptr, old_size > -1 ? old_size : num_bytes);
    } break;

    case kf_AllocationType_FREE: {
        kfd_printf("WARN: <temp> Could not FREE");
    } break;

    case kf_AllocationType_FREE_ALL: {
        data->position = 0;
    } break;

    case kf_AllocationType_QUERY_AVAIL_SPACE: {
        return (data->size - data->position);
    } break;

    default:
        KF_PANIC();

    }

    return out;
}

void kf_init_temp_allocator_data(kf_TempAllocatorData *data, kf_Allocator backing, isize capacity)
{
    data->start = (u8 *)kf_alloc(backing, kf_align_ceil(capacity));
    data->position = 0;
    data->size = capacity;
    data->num_allocations = 0;
}

kf_Allocator kf_temp_allocator(kf_TempAllocatorData *data)
{
    kf_Allocator out;
    out.allocate = kf_temp_allocator_proc;
    out.user = data;
    _set_alloc_type(&out, kf_AllocatorType_TEMP);
    return out;
}

/* PERMA ALLOCATOR */
void kf_init_perma_allocator_data(kf_PermaAllocatorData *data, kf_Allocator backing, isize initial_num_blocks, isize block_size, isize blocks_to_add_on_resize)
{
    data->blocks = (u8 *)kf_alloc(backing, initial_num_blocks * block_size);
    data->backing = backing;
    data->num_blocks = initial_num_blocks;
    data->blocks_to_add_on_resize = blocks_to_add_on_resize;
    data->block_size = block_size;

    /* (bool) */
    data->block_record = kf_array_make_length_capacity(backing, sizeof(bool), initial_num_blocks * 2, initial_num_blocks * 2);
}

void *kf_perma_allocator_proc(kf_Allocator alloc, kf_AllocationType type, void *ptr, isize num_bytes, isize old_size)
{
    void *out = NULL;
    kf_PermaAllocatorData *data = (kf_PermaAllocatorData *)alloc.user;

    isize aligned_bytes = kf_align_ceil(num_bytes);

    switch (type) {
    case kf_AllocationType_ALLOC: {
        if (aligned_bytes < data->block_size) {
            /* Can allocate within a single block */

        }
    } break;

    case kf_AllocationType_RESIZE: {
        KF_PANIC("Perma RESIZE not implemented");
    } break;

    case kf_AllocationType_FREE: {
        KF_PANIC("Could not FREE: is temp allocator");
    } break;

    case kf_AllocationType_FREE_ALL: {
        free(ptr);
    } break;

    case kf_AllocationType_QUERY_AVAIL_SPACE: {
        KF_PANIC("Perma QUERY_AVAIL_SPACE not implemented");
    } break;

    default:
        KF_PANIC();

    }

    return out;
}

kf_Allocator kf_perma_allocator(kf_PermaAllocatorData *data)
{
    kf_Allocator out;
    out.allocate = kf_perma_allocator_proc;
    out.user = data;
    _set_alloc_type(&out, kf_AllocatorType_PERMA);
    return out;
}