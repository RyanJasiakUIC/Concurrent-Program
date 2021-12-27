#include "hw6.h"
#include <stdio.h>
#include<pthread.h>

pthread_mutex_t lock;
pthread_cond_t passenger_has_exited = PTHREAD_COND_INITIALIZER;
pthread_cond_t passenger_has_entered = PTHREAD_COND_INITIALIZER;
int is_occupied = 0;
enum {yes = 1, no = 0} condition = no;
enum {yes2 = 1, no2 = 0} condition2 = no2;


int current_floor;
int direction;
int occupancy;
enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;

int pas_req[PASSENGERS];
int next_requested_index = 0;
int num_requests = 0;

int passengers_riding[PASSENGERS];
int current_passenger = -1;

void scheduler_init() {	
    current_floor=0;		
    direction=-1;
    occupancy=0;
    condition = no;
    state=ELEVATOR_ARRIVED;
    pthread_mutex_init(&lock,0);
    for (int i = 0; i < PASSENGERS; i++)
        pas_req[i] = -1;
}

void add_request(int from, int passenger) {
    pas_req[passenger] = from;
}

int get_next_request() {
    if (next_requested_index >= PASSENGERS)
        next_requested_index = 0;
    int n = 0;
    while (pas_req[next_requested_index] == -1) {
        next_requested_index++;
        if (next_requested_index >= PASSENGERS)
            next_requested_index = 0;
        if (++n > PASSENGERS)
            return 0;
    }
    return pas_req[next_requested_index++];
}

void remove_request(int passenger) {
    pas_req[passenger] = -1;
}

void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{	
    add_request(from_floor, passenger);
    num_requests++;
    // if (num_requests == 1) current_floor = from_floor;
    // printf("curent floor: %d\n", current_floor);
    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    int x = 0;
    while(waiting) {
        pthread_mutex_lock(&lock);

        if(current_floor == from_floor && state == ELEVATOR_OPEN && occupancy==0) {
            enter(passenger, 0);
            current_passenger = passenger;
            passengers_riding[current_passenger] = to_floor;
            occupancy++;
            waiting=0;
            condition = yes;
            pthread_cond_signal(&passenger_has_entered);
        }
        // if (x++ % 1000 == 0)
        //     printf("curent floor: %d\n", current_floor);
        if (current_floor == 0) {
            // if (x++ < 30)
            // printf("curent floor: %d\n", current_floor);
            condition = yes;
            pthread_cond_signal(&passenger_has_entered);
        }
        pthread_mutex_unlock(&lock);

        // pthread_mutex_unlock(&lock);
        // condition = yes;
        // pthread_cond_signal(&passenger_has_entered);
        // pthread_cond_signal(&passenger_has_entered);
    }

    // wait for the elevator at our destination floor, then get out
    int riding=1;
            // printf("WHAT\n");
    while(riding) {
        pthread_mutex_lock(&lock);
        // printf("What teh heck!\n");
        
            // printf("WHAT\n");
        if(current_floor == to_floor && state == ELEVATOR_OPEN) {
            // printf("***\ncurren_floor: %d\nto_floor: %d\n***\n", current_floor, to_floor);
            exit(passenger, 0);
            occupancy--;
            current_passenger = -1;
            num_requests--;
            remove_request(passenger);
            // printf("occupancy: %d\n", occupancy);
            riding=0;
        }
        pthread_mutex_unlock(&lock);
        condition2 = yes2;
        pthread_cond_signal(&passenger_has_exited);
    }
}

int passenger_is_waiting_at_floor(int floor)  {
    for (int i = 0; i < PASSENGERS; i++) {
        if (pas_req[i] == floor)
            return 1;
    }
    return 0;
}

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)) {
    if(elevator!=0) return;
    if(num_requests==0) return;
    // printf("read\n!!\n");
    pthread_mutex_lock(&lock);
    if(state == ELEVATOR_ARRIVED) {
        // printf("BRUH WHY\n");
        door_open(elevator);
        state=ELEVATOR_OPEN;
    }
    else if(state == ELEVATOR_OPEN) {
        if (occupancy == 0 && num_requests > 0) {
            condition = no;
            // printf("no occupancy, num_requests: %d!!!!!!!!\n", num_requests);
            while (condition == no && num_requests > 0 && passenger_is_waiting_at_floor(at_floor))
                pthread_cond_wait(&passenger_has_entered, &lock);
        } else if (current_passenger != -1 && passengers_riding[current_passenger] == at_floor) {
            condition2 = no2;
            while (condition2 == no2)
                pthread_cond_wait(&passenger_has_exited, &lock);
        }
        door_close(elevator);
        state=ELEVATOR_CLOSED;
    }
    else {
        int old_direction = direction;
        if (occupancy != 0) {
            int to_floor = passengers_riding[current_passenger];
            direction = to_floor-at_floor;
        } 
        else if (num_requests > 0) {
            int to_floor = get_next_request();
            // printf("NEXT FLOOR: %d\n", to_floor);
            direction = to_floor-at_floor;
        }
        else if (at_floor<=0 || at_floor>=FLOORS-1) {
            direction*=-1;
            // printf("direction: %d\n", direction);
        }
        if (at_floor + direction < FLOORS && at_floor+direction > -1)
            move_direction(elevator,direction);
        current_floor=at_floor+direction;
        if (direction != 1 && direction != -1) {
            direction = old_direction;
            if (at_floor == 0)
                direction = 1;
            else if (at_floor >= FLOORS-1)
                direction = -1;
        }
        state=ELEVATOR_ARRIVED;
        // printf("ELEVATOR ARRIVED\n");
    }
    pthread_mutex_unlock(&lock);
}