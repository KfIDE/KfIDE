void kf_write_current_time(kf_Time *t)
{
	time(&t->_time);
}

void kf_print_time_since(kf_Time *t)
{
	time_t end;
	double _diff;

	time(&end);
	_diff = difftime(end, t->_time);
	printf("TIME: %f\n", _diff);
}