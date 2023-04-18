kf_IRect kf_apply_rect_margin(kf_IRect r, kf_IRect margin)
{
	kf_IRect out;

	out.x = r.x + margin.x;
	out.y = r.y + margin.y;
	out.w = r.w - margin.w;
	out.h = r.h - margin.h;
	return out;
}

kf_IRect kf_apply_rect_translation(kf_IRect r, isize x, isize y)
{
	kf_IRect out;

	out.x = r.x + x;
	out.y = r.y + y;
	out.w = r.w;
	out.h = r.h;
	return out;
}

kf_UVRect kf_screen_to_uv(kf_IRect viewport, kf_IRect r)
{
	kf_UVRect out;

	out.x = (f32)(r.x + viewport.x) / (f32)viewport.w * 2.0f - 1.0f;
	out.y = (f32)(r.y + viewport.y) / (f32)viewport.h * -2.0f + 1.0f;
	out.x2 = (f32)(r.w + r.x) / (f32)viewport.w * 2.0f - 1.0f;
	out.y2 = (f32)(r.h + r.y) / (f32)viewport.h * -2.0f + 1.0f;
	return out;
}