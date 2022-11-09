#include <pthread.h>

struct station {
	int passengers_waiting_count; // Number Of Passengers Waiting
	int available_seats; // Number Of Available Seats
	int passengers_boarded; // Number Passengers Boarded 
	pthread_mutex_t passenger_mutex; // Mutex To Lock Critical Section 
	pthread_cond_t train_arrived_cond; // Conditional Variable To Allow Passengers On Waiting To Board 
	pthread_cond_t on_board_cond; // Conditional Variable To Let Train Know That Passengers Are On Board
	
};

void station_init(struct station *station);

void station_load_train(struct station *station, int count);

void station_wait_for_train(struct station *station);

void station_on_board(struct station *station);