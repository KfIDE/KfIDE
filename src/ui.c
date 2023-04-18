static IRect _current_origin(UIContext *ctx)
{
	GB_ASSERT(gb_array_count(ctx->origin_stack) > 0);
	return ctx->origin_stack[gb_array_count(ctx->origin_stack) - 1];
}

static void _write_cmd_header(UIDrawCommand *cmd, UIDrawType type, Color color)
{
	cmd->common.type = type;
	cmd->common.color = color;
}

static void _push_cmd(UIContext *ctx, UIDrawCommand cmd)
{
	gb_array_append(ctx->draw_commands, cmd);
}

void kf_ui_begin(UIContext *ctx, gbAllocator alloc, isize expected_num_draw_commands, EventState *ref_state)
{
	GB_ASSERT_NOT_NULL(ctx);
	GB_ASSERT(expected_num_draw_commands > 0);

	ctx->ref_state = ref_state;

	ctx->begin_allocator = alloc;
	gb_array_init_reserve(ctx->draw_commands, ctx->begin_allocator, expected_num_draw_commands);
	gb_array_init_reserve(ctx->origin_stack, ctx->begin_allocator, expected_num_draw_commands);

	ctx->margin = (IRect){ 0, 0, 0, 0 };
	ctx->color = KF_RGBA(255, 255, 255, 255);

	/* Push null origin */
	kf_ui_push_origin(ctx, 0, 0);
}

void kf_ui_end(UIContext *ctx, bool free_memory)
{
	GB_ASSERT_NOT_NULL(ctx);

	if (free_memory) {
		gb_free(ctx->begin_allocator, ctx->draw_commands);
		gb_free(ctx->begin_allocator, ctx->origin_stack);
	}
}

void kf_ui_push_origin(UIContext *ctx, isize x, isize y)
{
	IRect new_origin = (IRect){ x, y, 0, 0 };
	gb_array_append(ctx->origin_stack, new_origin); /* now this rect is the top of the stack */
}

void kf_ui_push_origin_relative(UIContext *ctx, isize x, isize y)
{
	IRect ctx_rect = _current_origin(ctx);
	kf_ui_push_origin(ctx, ctx_rect.x + x, ctx_rect.y + y);
}

void kf_ui_pop_origin(UIContext *ctx)
{
	gb_array_pop(ctx->origin_stack);
}

void kf_ui_color(UIContext *ctx, Color color)
{
	GB_ASSERT_NOT_NULL(ctx);

	ctx->color = color;
}

void kf_ui_margin(UIContext *ctx, IRect margin)
{
	GB_ASSERT_NOT_NULL(ctx);

	ctx->margin = margin;
}




/* Actual UI elements */
void kf_ui_rect(UIContext *ctx, IRect rect)
{
	UIDrawCommand cmd;

	_write_cmd_header(&cmd, KF_DRAW_RECT, ctx->color);
}