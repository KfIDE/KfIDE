#include "kf.h" /* NOTE(EimaMei): vscode stfu */

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
    KF_ASSERT(alloc.allocate != NULL && num_bytes >= 0);
    return ((kf_AllocatorProc)alloc.allocate) (alloc, KF_ALLOC_TYPE_ALLOC, NULL, num_bytes, -1);
}

void *kf_resize(kf_Allocator alloc, void *ptr, isize num_bytes, /*@ErrVal(-1)*/isize old_size)
{
    KF_ASSERT(alloc.allocate != NULL && ptr != NULL && num_bytes >= 0);

    if (old_size < 0) {
        old_size = num_bytes;
    }
    return ((kf_AllocatorProc)alloc.allocate) (alloc, KF_ALLOC_TYPE_RESIZE, ptr, num_bytes, old_size);
}

void kf_free(kf_Allocator alloc, void *ptr)
{
    ((kf_AllocatorProc)alloc.allocate) (alloc, KF_ALLOC_TYPE_FREE, ptr, -1, -1);
}

void kf_free_all(kf_Allocator alloc)
{
    ((kf_AllocatorProc)alloc.allocate) (alloc, KF_ALLOC_TYPE_FREE_ALL, NULL, -1, -1);
}

isize kf_query_avail_space(kf_Allocator alloc)
{
    void *res = ((kf_AllocatorProc)alloc.allocate) (alloc, KF_ALLOC_TYPE_QUERY_AVAIL_SPACE, NULL, -1, -1);
    if (res == NULL) {
        /* Could not query */
    } else {
        return (isize)res;
    }
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
kf_Array kf_array_make(kf_Allocator alloc, isize type_width, isize initial_length, isize initial_capacity, isize grow)
{
    kf_Array out;
    out.ptr = (u8 *)kf_alloc(alloc, type_width * initial_capacity);
    out.backing = alloc;
    out.type_width = type_width;
    out.length = initial_length;
    out.capacity = initial_capacity;
    out.grow = grow;
    return out;
}

kf_Array kf_array_set_frozen_from_ptr(void *ptr, isize type_width, isize num_elements)
{
    KF_ASSERT(ptr != NULL);

    kf_Array out;
    out.ptr = (u8 *)ptr;
    out.backing = (kf_Allocator){0}; /* frozen */
    out.type_width = type_width;
    out.length = num_elements;
    out.capacity = num_elements;
    return out;
}

kf_Array kf_array_set_dynamic_from_ptr(kf_Allocator alloc, void *ptr, isize type_width, isize num_elements, isize initial_capacity, isize grow)
{
    kf_Array out = kf_array_set_frozen_from_ptr(ptr, type_width, num_elements);
    out.backing = alloc;
    out.capacity = initial_capacity;
    out.grow = grow;
    return out;
}

void *kf_array_get(kf_Array array, isize index)
{
    KF_ASSERT(index >= 0 && index < array.length);

    return &array.ptr[index * array.type_width];
}

void *kf_array_get_type(kf_Array array, isize index, isize type_width)
{
    KF_ASSERT(type_width > 0 && type_width == array.type_width);
    return kf_array_get(array, index);
}

void kf_array_set(kf_Array *array, isize index, u8 *data)
{
    KF_ASSERT(index >= 0 && index < array->length && !kf_is_frozen(*array));

    memcpy(&array->ptr[index * array->type_width], data, array->type_width);
}

isize kf_array_next_cap(kf_Array array)
{
    return array.capacity + array.grow;
}

void kf_array_resize(kf_Array *array, isize new_cap)
{
    KF_ASSERT(!kf_is_frozen(*array));

    array->ptr = (u8 *)kf_resize(array->backing, array->ptr, new_cap * array->type_width, array->capacity * array->type_width);
    array->capacity = new_cap;
}

void kf_array_append(kf_Array *array, void *new_element)
{
    kf_array_append_multi(array, new_element, 1);
}

void kf_array_append_multi(kf_Array *array, void *new_elements, isize num)
{
    KF_ASSERT_NOT_NULL(array);
    KF_ASSERT_NOT_NULL(new_elements);
    KF_ASSERT(num > 0);

    if (kf_is_frozen(*array)) {
        KF_PANIC("Cannot append to frozen array!");
    }

    isize new_length = array->length + num;
    if (new_length > array->capacity) {
        kf_array_resize(array, new_length);
    }

    memcpy(&array->ptr[array->length * array->type_width], new_elements, array->type_width * num);
    array->length = new_length;
}

void kf_array_pop(kf_Array *array)
{
    KF_ASSERT_NOT_NULL(array);
    KF_ASSERT(!kf_is_frozen(*array));

    if (array->length > 0) {
        array->length--;
    } else {
        kfd_printf("WARN: tried to kf_array_pop() but the array has no elements!");
    }
}

void kf_array_clear(kf_Array *array)
{
    KF_ASSERT_NOT_NULL(array);
    KF_ASSERT(!kf_is_frozen(*array));

    array->length = 0;
}

void kf_freeze(kf_Array *array)
{
    array->backing.allocate = (kf_AllocatorProc)NULL;
}

void kf_unfreeze(kf_Array *array, kf_Allocator alloc, isize capacity, isize grow)
{
    array->backing = alloc;
    array->capacity = capacity;
    array->grow = grow;
}

bool kf_is_frozen(kf_Array array)
{
    return (bool)(array.backing.allocate == NULL);
}

void kf_array_free(kf_Array array)
{
    kf_free(array.backing, array.ptr);
}

void kfd_array_print(kf_Array array)
{
#ifdef KF_DEBUG
    kfd_printf("\nARRAY DATA:\nSTART %p\nELSIZE %ld\nLENGTH %ld\nEL %s", array.ptr, array.type_width, array.length, (u8 *)kf_array_get(array, 0));
#endif
}

/* STRINGS */
kf_String kf_string_set_from_cstring(u8 *cstr)
{
    isize length = strlen(cstr);
    return kf_string_set_from_cstring_len(cstr, length);
}

kf_String kf_string_set_from_cstring_len(u8 *cstr, isize length)
{
    return kf_array_set_frozen_from_ptr(cstr, sizeof(*cstr), length);
}

kf_String kf_string_copy_from_cstring(kf_Allocator alloc, u8 *cstr, isize grow)
{
    isize len = strlen(cstr);
    return kf_string_copy_from_cstring_len(alloc, cstr, len, len, KF_DEFAULT_STRING_GROW);
}

kf_String kf_string_copy_from_cstring_len(kf_Allocator alloc, u8 *cstr, isize len, isize cap, isize grow)
{
    KF_ASSERT(alloc.allocate != NULL && len <= cap && cstr != NULL && len > -1 && cap > -1 && grow > 0);

    kf_String out = kf_string_make(alloc, len, cap, grow);
    memcpy(out.ptr, cstr, len);
    return out;
}

kf_String kf_string_copy_from_string(kf_Allocator alloc, kf_String to_clone)
{
    return kf_string_copy_from_string_len(alloc, to_clone, to_clone.length, to_clone.capacity, to_clone.grow);
}

kf_String kf_string_copy_from_string_len(kf_Allocator alloc, kf_String to_clone, isize len, isize cap, isize grow)
{
    KF_ASSERT(alloc.allocate != NULL && len <= cap && to_clone.ptr != NULL && len > -1 && cap > -1 && grow > 0);

    kf_String out = kf_string_make(alloc, len, cap, grow);
    memcpy(out.ptr, to_clone.ptr, len);
    return out;
}

kf_String kf_string_make(kf_Allocator alloc, isize len, isize cap, isize grow)
{
    KF_ASSERT(len > -1 && cap > -1 && grow > 0);

    return kf_array_make(alloc, sizeof(u8), len, cap, grow);
}

kf_String kf_string_make_zeroed(kf_Allocator alloc, isize length, isize capacity, isize grow)
{
    kf_String out = kf_string_make(alloc, length, capacity, grow);
    memset(out.ptr, 0, length);
    return out;
}

isize kf_string_next_cap(kf_String str)
{
    return str.capacity + str.grow;
}

void kf_string_resize(kf_String *str, isize new_cap)
{
    str->capacity = new_cap;
}

void kf_string_clear(kf_String *str)
{
    str->length = 0;
}

void kf_string_free(kf_String str)
{
    if (kf_is_frozen(str)) {
        KF_PANIC("Tried to kf_string_free() a frozen string! Unfreeze it first and try again.");
    }
    kf_free(str.backing, str.ptr);
}

void kf_string_set_char(kf_String *str, isize at, u8 new_char)
{
    KF_ASSERT(str != NULL && at >= 0 && at <= str->length && !kf_is_frozen(*str));

    str->ptr[at] = new_char;
}

void kf_string_write_cstring_at(kf_String *str, u8 *cstr, isize at)
{
    kf_string_write_cstring_at_len(str, cstr, at, strlen(cstr));
}

void kf_string_write_cstring_at_len(kf_String *str, u8 *cstr, isize at, isize length)
{
    KF_ASSERT(str != NULL && cstr != NULL && at >= 0 && at <= length);

    memcpy(&str->ptr[at], cstr, length);
}

void kf_string_write_string_at(kf_String *str, kf_String other, isize at)
{
    kf_string_write_cstring_at_len(str, other.ptr, at, other.length);
}

void kf_string_write_string_at_len(kf_String *str, kf_String other, isize at, isize length)
{
    kf_string_write_cstring_at_len(str, other.ptr, at, length);
}

void kf_string_append_cstring(kf_String *str, u8 *cstr)
{
    kf_string_append_cstring_len(str, cstr, strlen(cstr));
}

void kf_string_append_cstring_len(kf_String *str, u8 *cstr, isize length)
{
    KF_ASSERT(str != NULL && cstr != NULL);

    isize new_length = str->length + length;
    if (new_length > str->capacity) {
        kf_string_resize(str, new_length);
    }

    memcpy(&str->ptr[str->length], cstr, length);
    str->length = new_length;

    /*
        NOTE (EimaMei):
        According 📖✍ to my 🗑️ statistics ➗, only a small 👌⬇ percentage ➗ of people 👨 that program 🖥️ in C 🇨 actually 😳 know how to properly manage memory ✔,
        so if you 🤔👄👈 end 🔚 up ☝ learning 🐩 C 💾, consider 🤔 learning not to forget the basics of creating strings in C 📬, it's free 🆓, and you 👈 can always 🔥
        change 🚼 your 👉 mind 🤯. Enjoy 🌟💯 the code 📹.
    */
    str->ptr[new_length] = '\0';
}

void kf_string_append_string(kf_String *str, kf_String other)
{
    kf_string_append_cstring_len(str, other.ptr, other.length);
}

void kf_string_append_string_len(kf_String *str, kf_String other, isize slice_other_upto)
{
    kf_string_append_cstring_len(str, other.ptr, slice_other_upto);
}

u8 kf_string_pop(kf_String *str)
{
    return str->ptr[--str->length];
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
    case KF_ALLOC_TYPE_ALLOC: {
        out = malloc(aligned_bytes);
    } break;

    case KF_ALLOC_TYPE_RESIZE: {
        out = realloc(ptr, aligned_bytes);
    } break;

    case KF_ALLOC_TYPE_FREE: {
        free(ptr);
    } break;

    case KF_ALLOC_TYPE_FREE_ALL: {
        KF_PANIC("Could not FREE_ALL: is heap allocator");
    } break;

    case KF_ALLOC_TYPE_QUERY_AVAIL_SPACE: {
        return NULL;
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
    _set_alloc_type(&out, KF_ALLOC_HEAP);
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
    case KF_ALLOC_TYPE_ALLOC: {
        /* Sub-allocate within buffer */
        isize new_pos = data->position + aligned_bytes;
        if (new_pos >= data->size) {
            kfd_printf("<temp> Could not ALLOC: it exceeds max size!\nBuffer size: %d K\nMax avail: %d B\nYou wanted: %lld B", data->size / KF_KILO(1), (data->size - data->position), aligned_bytes);
            KF_PANIC();
        }

        out = &data->ptr[data->position];
        data->position = new_pos;
    } break;

    case KF_ALLOC_TYPE_RESIZE: {
        /* This is very inefficient and bad
        You have to call the above _ALLOC code and then memcpy into the new block */
        out = kf_alloc(alloc, aligned_bytes);
        memcpy(out, ptr, old_size > -1 ? old_size : num_bytes);
    } break;

    case KF_ALLOC_TYPE_FREE: {
        kfd_printf("WARN: <temp> Could not FREE");
    } break;

    case KF_ALLOC_TYPE_FREE_ALL: {
        data->position = 0;
    } break;

    case KF_ALLOC_TYPE_QUERY_AVAIL_SPACE: {
        return (data->size - data->position);
    } break;

    default:
        KF_PANIC();

    }

    return out;
}

void kf_init_temp_allocator_data(kf_TempAllocatorData *data, kf_Allocator backing, isize capacity)
{
    data->ptr = (u8 *)kf_alloc(backing, kf_align_ceil(capacity));
    data->position = 0;
    data->size = capacity;
    data->num_allocations = 0;
}

kf_Allocator kf_temp_allocator(kf_TempAllocatorData *data)
{
    kf_Allocator out;
    out.allocate = kf_temp_allocator_proc;
    out.user = data;
    _set_alloc_type(&out, KF_ALLOC_TEMP);
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
    data->block_record = kf_array_make(backing, sizeof(bool), initial_num_blocks * 2, initial_num_blocks * 2, KF_DEFAULT_GROW);
}

void *kf_perma_allocator_proc(kf_Allocator alloc, kf_AllocationType type, void *ptr, isize num_bytes, isize old_size)
{
    void *out = NULL;
    kf_PermaAllocatorData *data = (kf_PermaAllocatorData *)alloc.user;

    isize aligned_bytes = kf_align_ceil(num_bytes);

    switch (type) {
    case KF_ALLOC_TYPE_ALLOC: {
        if (aligned_bytes < data->block_size) {
            /* Can allocate within a single block */

        }
    } break;

    case KF_ALLOC_TYPE_RESIZE: {
        KF_PANIC("Perma RESIZE not implemented");
    } break;

    case KF_ALLOC_TYPE_FREE: {
        KF_PANIC("Could not FREE: is temp allocator");
    } break;

    case KF_ALLOC_TYPE_FREE_ALL: {
        free(ptr);
    } break;

    case KF_ALLOC_TYPE_QUERY_AVAIL_SPACE: {
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
    _set_alloc_type(&out, KF_ALLOC_PERMA);
    return out;
}