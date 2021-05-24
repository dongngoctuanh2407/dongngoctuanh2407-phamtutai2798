#include "pintos_thread.h"
#include <stdio.h>

struct station {
	struct condition *cond_train_arrived;
	struct condition *cond_all_passengers_seated;
	struct lock *lck;
	int station_waiting_passengers;
	int train_empty_seats;
	int train_standing_passengers;
};


void print_station(struct station *station) 
{
	printf("[Ga tàu| Hành khách đang chờ: %d, Hành khách đang đứng trên tàu: %d, Chỗ trống trên tàu: %d]\n",
			station->station_waiting_passengers,station->train_standing_passengers,station->train_empty_seats);
}

// Hàm này sẽ được gọi để khởi tạo đối tượng trạm khi CalTrain khởi động
void station_init(struct station *station)
{
	station->cond_train_arrived = malloc(sizeof(struct condition));
	station->cond_all_passengers_seated= malloc(sizeof(struct condition));
	station->lck = malloc(sizeof(struct lock));
	cond_init(station->cond_train_arrived);
	cond_init(station->cond_all_passengers_seated);
	lock_init(station->lck);
	station->station_waiting_passengers = 0;
	station->train_empty_seats = 0;
	station->train_standing_passengers = 0;
	printf("init ->"); print_station(station);
}


/* Khi một đoàn tàu đến ga và đã mở cửa, nó sẽ gọi hàm
 * count cho biết có bao nhiêu chỗ ngồi trên tàu
 */
void station_load_train(struct station *station, int count)
{
	lock_acquire(station->lck);
	station->train_empty_seats = count;
	printf("Tàu đến (chỗ trống: %d)->", count); print_station(station);
/*Chức năng này không được hoạt động trở lại cho đến khi tàu được xếp đầy đủ 
* (tất cả hành khách đã ngồi vào ghế của họ, và tàu đã đầy hoặc tất cả hành khách đang chờ đã lên tàu)
*/
	while ((station->station_waiting_passengers > 0) && (station->train_empty_seats > 0)) {
		cond_broadcast(station->cond_train_arrived,station->lck);
		cond_wait(station->cond_all_passengers_seated,station->lck);
	}

	//all passengers boarded 
	printf("Tàu đã rời đi ->"); print_station(station);

	//reset for next train
	station->train_empty_seats = 0;
	lock_release(station->lck);
}


/* 
 * Khi rô bốt chở khách đến nhà ga, đầu tiên nó sẽ gọi chức năng
 * passenger on board the train and into a seat 
 */
 void station_wait_for_train(struct station *station)
{
	lock_acquire(station->lck);
	station->station_waiting_passengers++;
	printf("hành khách đến ->"); print_station(station);
/* Chức năng này không được hoạt động trở lại cho đến khi có tàu trong ga 
 * (tức là đang có một cuộc gọi đến station_load_train) và có đủ ghế trống trên tàu cho hành khách này ngồi xuống.
 */
	while (station->train_standing_passengers == station->train_empty_seats) 
		cond_wait(station->cond_train_arrived,station->lck);
	station->train_standing_passengers++;
	station->station_waiting_passengers--;
	printf("hành khách đang lên tàu ->"); print_station(station);
	lock_release(station->lck);

}


/* Khi chức năng này hoạt động trở lại, robot chở khách sẽ di chuyển hành khách trên tàu và vào một chỗ ngồi 
 * Khi hành khách đã yên vị, nó sẽ gọi hàm
 */
void station_on_board(struct station *station) //để cho tàu biết rằng nó đang ở trên tàu.
{
	lock_acquire(station->lck);
	station->train_standing_passengers--;
	station->train_empty_seats--;
	printf("hành khách trên tàu"); print_station(station);
	if ((station->train_empty_seats == 0) || (station->train_standing_passengers == 0))
		cond_signal(station->cond_all_passengers_seated,station->lck);
	
	lock_release(station->lck);
}







