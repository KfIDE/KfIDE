static kf_IRect _current_origin(kf_UIContext *ctx)
{
	KF_ASSERT((ctx->origin_stack).length > 0);
	
	kf_IRect *out = kf_array_get(ctx->origin_stack, (ctx->origin_stack).length - 1);
	return *out;
}

static kf_IRect _current_origin_plus(kf_UIContext *ctx, kf_IRect plus)
{
	kf_IRect origin = _current_origin(ctx);
	return KF_IRECT(origin.x + plus.x, origin.y + plus.y, plus.w, plus.h);
}

static void _write_cmd_header(kf_UIDrawCommand *cmd, kf_UIDrawType type, kf_Color color)
{
	cmd->common.type = type;
	cmd->common.color = color;
}

static void _push_cmd(kf_UIContext *ctx, kf_UIDrawCommand cmd)
{
	kf_array_append(&ctx->draw_commands, &cmd);
}

void kf_ui_begin(kf_UIContext *ctx, kf_Allocator alloc, isize expected_num_draw_commands, kf_EventState *ref_state)
{
	KF_ASSERT_NOT_NULL(ctx);
	KF_ASSERT(expected_num_draw_commands > 0);

	ctx->ref_state			= ref_state;

	ctx->begin_allocator	= alloc;
	ctx->draw_commands		= kf_array_make(ctx->begin_allocator, sizeof(kf_UIDrawCommand), 0, expected_num_draw_commands, KF_DEFAULT_GROW);
	ctx->origin_stack		= kf_array_make(ctx->begin_allocator, sizeof(kf_IRect), 0, expected_num_draw_commands, KF_DEFAULT_GROW);

	ctx->margin				= KF_IRECT(0, 0, 0, 0);
	ctx->color				= KF_RGBA(255, 255, 255, 255);

	/* Push null origin */
	kf_ui_push_origin(ctx, 0, 0);
}

void kf_ui_end(kf_UIContext *ctx, bool free_memory)
{
	KF_ASSERT_NOT_NULL(ctx);

	if (free_memory) {
		kf_array_free(ctx->draw_commands);
		kf_array_free(ctx->origin_stack);
	}
}

void kf_ui_push_origin(kf_UIContext *ctx, isize x, isize y)
{
	kf_IRect new_origin = (kf_IRect){ x, y, 0, 0 };
	kf_array_append(&ctx->origin_stack, &new_origin); /* now this rect is the top of the stack */
}

void kf_ui_push_origin_relative(kf_UIContext *ctx, isize x, isize y)
{
	kf_IRect ctx_rect = _current_origin(ctx);
	kf_ui_push_origin(ctx, ctx_rect.x + x, ctx_rect.y + y);
}

void kf_ui_pop_origin(kf_UIContext *ctx)
{
	kf_array_pop(&ctx->origin_stack);
}




/* Style settings */
void kf_ui_color(kf_UIContext *ctx, kf_Color color)
{
	KF_ASSERT_NOT_NULL(ctx);

	ctx->color = color;
}

void kf_ui_margin(kf_UIContext *ctx, kf_IRect margin)
{
	KF_ASSERT_NOT_NULL(ctx);

	ctx->margin = margin;
}

void kf_ui_font(kf_UIContext *ctx, kf_Font *font)
{
	KF_ASSERT_NOT_NULL(ctx);
	KF_ASSERT_NOT_NULL(font);

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

void kf_ui_text(kf_UIContext *ctx, kf_String text, isize x, isize y)
{
	kf_UIDrawCommand cmd;
	kf_IRect res = kf_apply_rect_margin(_current_origin_plus(ctx, KF_IRECT(x, y, 0, 0)), ctx->margin);;

	_write_cmd_header(&cmd, KF_DRAW_TEXT, ctx->color);
	cmd.text.text = text;
	cmd.text.font = ctx->font; KF_ASSERT(ctx->font != NULL);
	cmd.text.begin = (kf_IVector2){ res.x, res.y };
	_push_cmd(ctx, cmd);
}