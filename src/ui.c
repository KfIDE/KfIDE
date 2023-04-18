static kf_IRect _current_origin(kf_UIContext *ctx)
{
	GB_ASSERT(gb_array_count(ctx->origin_stack) > 0);
	return ctx->origin_stack[gb_array_count(ctx->origin_stack) - 1];
}

static kf_IRect _current_origin_plus(kf_UIContext *ctx, kf_IRect plus)
{
	kf_IRect origin = _current_origin(ctx);
	return (kf_IRect){ origin.x + plus.x, origin.y + plus.y, plus.w, plus.h };
}

static void _write_cmd_header(kf_UIDrawCommand *cmd, kf_UIDrawType type, kf_Color color)
{
	cmd->common.type = type;
	cmd->common.color = color;
}

static void _push_cmd(kf_UIContext *ctx, kf_UIDrawCommand cmd)
{
	gb_array_append(ctx->draw_commands, cmd);
}

void kf_ui_begin(kf_UIContext *ctx, gbAllocator alloc, isize expected_num_draw_commands, kf_EventState *ref_state)
{
	GB_ASSERT_NOT_NULL(ctx);
	GB_ASSERT(expected_num_draw_commands > 0);

	ctx->ref_state = ref_state;

	ctx->begin_allocator = alloc;
	gb_array_init_reserve(ctx->draw_commands, ctx->begin_allocator, expected_num_draw_commands);
	gb_array_init_reserve(ctx->origin_stack, ctx->begin_allocator, expected_num_draw_commands);

	ctx->margin = (kf_IRect){ 0, 0, 0, 0 };
	ctx->color = KF_RGBA(255, 255, 255, 255);

	/* Push null origin */
	kf_ui_push_origin(ctx, 0, 0);
}

void kf_ui_end(kf_UIContext *ctx, bool free_memory)
{
	GB_ASSERT_NOT_NULL(ctx);

	if (free_memory) {
		gb_free(ctx->begin_allocator, ctx->draw_commands);
		gb_free(ctx->begin_allocator, ctx->origin_stack);
	}
}

void kf_ui_push_origin(kf_UIContext *ctx, isize x, isize y)
{
	kf_IRect new_origin = (kf_IRect){ x, y, 0, 0 };
	gb_array_append(ctx->origin_stack, new_origin); /* now this rect is the top of the stack */
}

void kf_ui_push_origin_relative(kf_UIContext *ctx, isize x, isize y)
{
	kf_IRect ctx_rect = _current_origin(ctx);
	kf_ui_push_origin(ctx, ctx_rect.x + x, ctx_rect.y + y);
}

void kf_ui_pop_origin(kf_UIContext *ctx)
{
	gb_array_pop(ctx->origin_stack);
}




/* Style settings */
void kf_ui_color(kf_UIContext *ctx, kf_Color color)
{
	GB_ASSERT_NOT_NULL(ctx);

	ctx->color = color;
}

void kf_ui_margin(kf_UIContext *ctx, kf_IRect margin)
{
	GB_ASSERT_NOT_NULL(ctx);

	ctx->margin = margin;
}

void kf_ui_font(kf_UIContext *ctx, kf_Font *font)
{
	GB_ASSERT_NOT_NULL(ctx);
	GB_ASSERT_NOT_NULL(font);

	ctx->font = font;
}




/* Actual UI elements */
void kf_ui_rect(kf_UIContext *ctx, kf_IRect rect)
{
	kf_UIDrawCommand cmd;

	_write_cmd_header(&cmd, KF_DRAW_RECT, ctx->color);
	cmd.rect.rect = kf_apply_rect_margin(_current_origin_plus(ctx, rect), ctx->margin);
	_push_cmd(ctx, cmd);
}

void kf_ui_text(kf_UIContext *ctx, gbString text, isize x, isize y)
{
	kf_UIDrawCommand cmd;

	_write_cmd_header(&cmd, KF_DRAW_TEXT, ctx->color);
	cmd.text.text = text;
	cmd.text.font = ctx->font; GB_ASSERT(ctx->font != NULL);
	cmd.text.begin = (kf_IVector2){ x, y };
	_push_cmd(ctx, cmd);
}