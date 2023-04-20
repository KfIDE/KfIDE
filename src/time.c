void kf_write_current_time(kf_Time *t)
{
	t->start_t = clock();
}

void kf_print_time_since(kf_Time *t)
{
	time_t end;
	double diff;

	end = clock();
	diff = (double)(end - t->start_t) / CLOCKS_PER_SEC;
	printf("TIME: %f\n", diff * 1000.0f);
}