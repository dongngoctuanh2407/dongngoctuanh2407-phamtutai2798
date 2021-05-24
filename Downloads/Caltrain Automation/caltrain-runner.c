#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "caltrain.c"

volatile int threads_completed = 0;

void*
passenger_thread(void *arg)
{
	struct station *station = (struct station*)arg;
	station_wait_for_train(station);
	__sync_add_and_fetch(&threads_completed, 1);
	return NULL;
}

struct load_train_args {
	struct station *station;
	int free_seats;
};
// để biến có thể thay đổi mà không báo trước
volatile int load_train_returned = 0;

void*
load_train_thread(void *args)
{
	struct load_train_args *ltargs = (struct load_train_args*)args;
	station_load_train(ltargs->station, ltargs->free_seats);
	load_train_returned = 1;
	return NULL;
}

const char* alarm_error_str;
int alarm_timeout;
void
_alarm(int seconds, const char *error_str)
{
	alarm_timeout = seconds;
	alarm_error_str = error_str;
	alarm(seconds);
}

void
alarm_handler(int foo)
{
	fprintf(stderr, "Error: Không thể hoàn thành sau %d giây."
		"sai hoặc hệ thống của bạn quá chậm. Gợi ý lỗi có thể xảy ra: [%s]\n",
		alarm_timeout, alarm_error_str);
	exit(1);
}

#ifndef MIN
#define MIN(_x,_y) ((_x) < (_y)) ? (_x) : (_y)
#endif

/*
 * Tạo ra một loạt các chuỗi để mô phỏng các chuyến tàu đến và hành khách.
 */
int
main()
{
	printf("----Project 0: Caltrain Automation----\n");
	struct station station;
	station_init(&station);

	srandom(getpid() ^ time(NULL));

	signal(SIGALRM, alarm_handler);

	// Đảm bảo rằng station_load_train () trả về ngay lập tức nếu không có hành khách nào đang chờ.
	_alarm(1, "station_load_train() không trở lại ngay lập tức khi không có hành khách đợi");
	station_load_train(&station, 0);
	station_load_train(&station, 10);
	_alarm(0, NULL);

	// Tạo một nhóm 'hành khách', mỗi hành khách trong chuỗi riêng của họ
	int i;
	const int total_passengers = 1000;
	int passengers_left = total_passengers;
	for (i = 0; i < total_passengers; i++) {
		pthread_t tid;
		int ret = pthread_create(&tid, NULL, passenger_thread, &station);
		if (ret != 0) {
			// Nếu điều này không thành công,có lẽ đã vượt quá giới hạn hệ thống.
			// Thử giảm 'total_passengers'.
			perror("pthread_create");
			exit(1);
		}
	}

	// Đảm bảo rằng station_load_train () trả lại ngay lập tức nếu không có ghế trống.
	_alarm(2, "station_load_train() không quay lại ngay lập tức khi không có chỗ ngồi miễn phí");
	station_load_train(&station, 0);
	_alarm(0, NULL);

	// kiểm tra ngẫu nhiên.
	int total_passengers_boarded = 0;
	const int max_free_seats_per_train = 50;
	int pass = 0;
	while (passengers_left > 0) {
		_alarm(2, "một số vấn đề khác  "
			"hành khách không lên tàu khi có cơ hội");

		int free_seats = random() % max_free_seats_per_train;

		printf("Tàu vào ga với  %d chỗ trống\n", free_seats);
		load_train_returned = 0;
		struct load_train_args args = { &station, free_seats };
		pthread_t lt_tid;
		int ret = pthread_create(&lt_tid, NULL, load_train_thread, &args);
		if (ret != 0) {
			perror("pthread_create");
			exit(1);
		}

		int threads_to_reap = MIN(passengers_left, free_seats);
		printf("số chỗ trống dự kiến: %d, số hành khách dời đi dự kiến: %d\n",free_seats,passengers_left); 
		int threads_reaped = 0;
		while (threads_reaped < threads_to_reap) {
			if (load_train_returned) {
				fprintf(stderr, "Error: station_load_train returned early!\n");
				exit(1);
			}
			if (threads_completed > 0) {
				if ((pass % 2) == 0)
					usleep(random() % 2);
				threads_reaped++;
				station_on_board(&station);
				__sync_sub_and_fetch(&threads_completed, 1);
			}
		}

		// Chờ một chút nữa. Cho station_load_train () cơ hội quay lại
		// và đảm bảo rằng không có thêm hành khách nào lên tàu. Một giây
		// thời gian sẽ là hàng tấn, nhưng nếu bạn đang sử dụng một hệ thống quá tải khủng khiếp,
		// điều này có thể cần được điều chỉnh.
		for (i = 0; i < 1000; i++) {
			if (i > 50 && load_train_returned)
				break;
			usleep(1000);
		}

		if (!load_train_returned) {
			fprintf(stderr, "Error: station_load_train không trở lại được\n");
			exit(1);
		}

		while (threads_completed > 0) {
			threads_reaped++;
			__sync_sub_and_fetch(&threads_completed, 1);
		}

		passengers_left -= threads_reaped;
		total_passengers_boarded += threads_reaped;
		printf("Ga tàu khởi hành với %d hành khách mới (dự kiến %d)%s\n",
			threads_to_reap, threads_reaped,
			(threads_to_reap != threads_reaped) ? " *****" : "");

		if (threads_to_reap != threads_reaped) {
			fprintf(stderr, "Error: Quá nhiều hành khách trên chuyến tàu này!\n");
			exit(1);
		}

		pass++;
	}

	if (total_passengers_boarded == total_passengers) {
		printf("Finshed !\n");
		return 0;
	} else {
		// Điều này khó có thể xảy ra, nhưng đề phòng
		fprintf(stderr, "Error: dự kiến %d tổng số hành khách lên tàu, nhưng chỉ có %d!\n",
			total_passengers, total_passengers_boarded);
		return 1;
	}
}
