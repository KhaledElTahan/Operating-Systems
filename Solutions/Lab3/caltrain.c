#include <pthread.h>
#include "caltrain.h"



void
station_init(struct station *station)
{
	/*
	* Initialize Variables
	* Initialize Mutex
	* Initailize Conditional Variables
	*/
	station->passengers_waiting_count = 0;
	station->available_seats = 0;
	station->passengers_boarded = 0;
	pthread_mutex_init(&station->passenger_mutex, NULL);
	pthread_cond_init(&station->train_arrived_cond, NULL);
	pthread_cond_init(&station->on_board_cond, NULL);
}

void
station_load_train(struct station *station, int count)
{
	pthread_mutex_lock(&station->passenger_mutex);

	// Return Promptly If There Isn't Available Seats Or There Is No Passengers
	if (count == 0 || station->passengers_waiting_count == 0) {
		pthread_mutex_unlock(&station->passenger_mutex);
		return;
	}	
	// Assign Available Seats
	station->available_seats = count;	
	// Allow Multiple Passengers To Board Simultaneously
	pthread_cond_broadcast(&station->train_arrived_cond);
	// The Train Waits For Signal To Leave
	pthread_cond_wait(&station->on_board_cond, &station->passenger_mutex);
	
	pthread_mutex_unlock(&station->passenger_mutex);
}

void
station_wait_for_train(struct station *station)
{
	pthread_mutex_lock(&station->passenger_mutex);
	// Add New Passenger On The Station
	station->passengers_waiting_count++;
	// Make The Passenger Wait Until A Train Arrive
	while(station->available_seats == 0) {
		pthread_cond_wait(&station->train_arrived_cond, &station->passenger_mutex);
	}
	// Decrement Passengers Count And Available Seats
	station->passengers_waiting_count--;
	station->available_seats--;	
	// Increment Boarded Passengers
	station->passengers_boarded++;
	
	pthread_mutex_unlock(&station->passenger_mutex);
}

void
station_on_board(struct station *station)
{
	pthread_mutex_lock(&station->passenger_mutex);
	// Increment Boarded Passengers To Let The Train When The Passenger Board
	station->passengers_boarded--;
	if (!station->passengers_boarded && !(station->available_seats && station->passengers_waiting_count)) {
		// Send A Signal To Train To Know That All Passengers Boarded Or No Available Seats
		pthread_cond_signal(&station->on_board_cond);
	}

	pthread_mutex_unlock(&station->passenger_mutex);

}
