#include "hw6.h"
#include <stdio.h>
#include<pthread.h>

pthread_mutex_t lock;
pthread_cond_t passenger_has_exited = PTHREAD_COND_INITIALIZER;
pthread_cond_t passenger_has_entered = PTHREAD_COND_INITIALIZER;
int is_occupied = 0;

typedef struct ELEVATOR_INFO el_info;
struct ELEVATOR_INFO {
    int current_floor;
    int direction;
    int occupancy;
    enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;	
    enum {yes = 1, no = 0} condition;
    enum {yes2 = 1, no2 = 0} condition2;
}; 

el_info elevators[ELEVATORS];

void scheduler_init() {	
    for (int i = 0; i < ELEVATORS; i++) {
        elevators[i].current_floor=0;		
        elevators[i].direction=-1;
        elevators[i].occupancy=0;
        elevators[i].condition = no;
        elevators[i].condition2 = no2;
        elevators[i].state=ELEVATOR_ARRIVED;
        pthread_mutex_init(&lock,0);
    }
}

int passengers_riding[PASSENGERS];
int current_passenger = -1;
int num_requests = 0;

void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{	
    num_requests++;
    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    while(waiting) {
        pthread_mutex_lock(&lock);

        if(current_floor == from_floor && state == ELEVATOR_OPEN && occupancy==0) {
            enter(passenger, 0);
            current_passenger = passenger;
            passengers_riding[current_passenger] = to_floor;
            occupancy++;
            waiting=0;
        }

        pthread_mutex_unlock(&lock);
        condition = yes;
        pthread_cond_signal(&passenger_has_entered);
        // pthread_cond_signal(&passenger_has_entered);
    }

    // wait for the elevator at our destination floor, then get out
    int riding=1;
            // printf("WHAT\n");
    while(riding) {
        pthread_mutex_lock(&lock);

        
            // printf("WHAT\n");
        if(current_floor == to_floor && state == ELEVATOR_OPEN) {
            // printf("***\ncurren_floor: %d\nto_floor: %d\n***\n", current_floor, to_floor);
            exit(passenger, 0);
            occupancy--;
            current_passenger = -1;
            printf("occupancy: %d\n", occupancy);
            riding=0;
        }
        pthread_mutex_unlock(&lock);
        condition2 = yes2;
        pthread_cond_signal(&passenger_has_exited);
    }
    num_requests--;
}

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)) {
    if(elevator!=0) return;
    if(num_requests==0) return;
    printf("read\n!!\n");
    pthread_mutex_lock(&lock);
    if(state == ELEVATOR_ARRIVED) {
        door_open(elevator);
        state=ELEVATOR_OPEN;
    }
    else if(state == ELEVATOR_OPEN) {
        if (occupancy == 0 && num_requests > 0) {
            condition = no;
            printf("no occupancy, num_requests: %d\n", num_requests);
            while (condition == no && num_requests > 0)
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
        else if(at_floor<=0 || at_floor>=FLOORS-1) {
            direction*=-1;
            printf("direction: %d\n", direction);
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
        // else if(at_floor==0 || at_floor==FLOORS-1) 
        //     direction*=-1;
        state=ELEVATOR_ARRIVED;
        printf("ELEVATOR ARRIVED\n");
    }
    pthread_mutex_unlock(&lock);
}