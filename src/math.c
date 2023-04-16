IRect kf_apply_rect_margin(IRect r, IRect margin)
{
	IRect out;

	out.x = r.x + margin.x;
	out.y = r.y + margin.y;
	out.w = r.w - margin.w*2;
	out.h = r.h - margin.h*2;
	return out;
}

IRect kf_apply_rect_translation(IRect r, isize x, isize y)
{
	IRect out;

	out.x = r.x + x;
	out.y = r.y + y;
	out.w = r.w;
	out.h = r.h;
	return out;
}