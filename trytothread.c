#include "hw6.h"
#include <stdio.h>
#include<pthread.h>

pthread_mutex_t lock;
// pthread_cond_t passenger_has_exited[ELEVATORS] = PTHREAD_COND_INITIALIZER;
// pthread_cond_t passenger_has_entered[ELEVATORS] = PTHREAD_COND_INITIALIZER;
int is_occupied = 0;

typedef struct ELEVATOR_INFO el_info;
struct ELEVATOR_INFO {
    int current_floor;
    int direction;
    int occupancy;
    enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;	
    enum {yes = 1, no = 0} condition;
    enum {yes2 = 1, no2 = 0} condition2;
    int passengers_riding[PASSENGERS];
    int current_passenger;
    pthread_cond_t passenger_has_exited;
    pthread_cond_t passenger_has_entered;
}; 

int num_requests = 0;

el_info e[ELEVATORS];

void scheduler_init() {	
    for (int i = 0; i < ELEVATORS; i++) {
        e[i].current_floor=0;		
        e[i].direction=-1;
        e[i].occupancy=0;
        e[i].condition = no;
        e[i].condition2 = no2;
        e[i].state = ELEVATOR_ARRIVED;
        e[i].current_passenger = -1;
        pthread_cond_init(&(e[i].passenger_has_entered), NULL);
        pthread_cond_init(&(e[i].passenger_has_exited), NULL);
    }
    pthread_mutex_init(&lock,0);
}

int next_elev = 0;
int next_elevator() {
    if (next_elev < ELEVATORS) return next_elev++;
    next_elev = 1;
    return 0;
}

void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int)) {	
    num_requests++;
    int elev = next_elevator();
    printf("asign passenger: %d to elev: %d\n", passenger, elev);
    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    while(waiting) {
        pthread_mutex_lock(&lock);

        if(e[elev].current_floor == from_floor && e[elev].state == ELEVATOR_OPEN && e[elev].occupancy==0) {
            enter(passenger, 0);
            e[elev].current_passenger = passenger;
            e[elev].passengers_riding[e[elev].current_passenger] = to_floor;
            e[elev].occupancy++;
            waiting=0;
        }

        pthread_mutex_unlock(&lock);
        e[elev].condition = yes;
        pthread_cond_signal(&(e[elev].passenger_has_entered));
        // pthread_cond_signal(&passenger_has_entered);
    }

    // wait for the elevator at our destination floor, then get out
    int riding=1;
            // printf("WHAT\n");
    while(riding) {
        pthread_mutex_lock(&lock);

        
            // printf("WHAT\n");
        if(e[elev].current_floor == to_floor && e[elev].state == ELEVATOR_OPEN) {
            // printf("***\ncurren_floor: %d\nto_floor: %d\n***\n", current_floor, to_floor);
            exit(passenger, 0);
            e[elev].occupancy--;
            e[elev].current_passenger = -1;
            printf("occupancy: %d\n", e[elev].occupancy);
            riding=0;
        }
        pthread_mutex_unlock(&lock);
        e[elev].condition2 = yes2;
        pthread_cond_signal(&(e[elev].passenger_has_exited));
    }
    num_requests--;
}

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)) {
    // if(elevator!=0) return;
    if(num_requests==0) return;
    printf("read\n!!\n");
    pthread_mutex_lock(&lock);
    if(e[elevator].state == ELEVATOR_ARRIVED) {
        door_open(elevator);
        e[elevator].state=ELEVATOR_OPEN;
    }
    else if(e[elevator].state == ELEVATOR_OPEN) {
        if (e[elevator].occupancy == 0 && num_requests > 0) {
            e[elevator].condition = no;
            printf("no occupancy, num_requests: %d\n", num_requests);
            while (e[elevator].condition == no && num_requests > 0)
                pthread_cond_wait(&e[elevator].passenger_has_entered, &lock);
        } else if (e[elevator].current_passenger != -1 && e[elevator].passengers_riding[e[elevator].current_passenger] == at_floor) {
            e[elevator].condition2 = no2;
            while (e[elevator].condition2 == no2)
                pthread_cond_wait(&e[elevator].passenger_has_exited, &lock);
        }
        door_close(elevator);
        e[elevator].state=ELEVATOR_CLOSED;
    }
    else {
        int old_direction = e[elevator].direction;
        if (e[elevator].occupancy != 0) {
            int to_floor = e[elevator].passengers_riding[e[elevator].current_passenger];
            e[elevator].direction = to_floor-at_floor;
        } 
        else if(at_floor<=0 || at_floor>=FLOORS-1) {
            e[elevator].direction*=-1;
            printf("direction: %d\n", e[elevator].direction);
        }
        if (at_floor + e[elevator].direction < FLOORS && at_floor+e[elevator].direction > -1)
            move_direction(elevator,e[elevator].direction);
        e[elevator].current_floor=at_floor+e[elevator].direction;
        if (e[elevator].direction != 1 && e[elevator].direction != -1) {
            e[elevator].direction = old_direction;
            if (at_floor == 0)
                e[elevator].direction = 1;
            else if (at_floor >= FLOORS-1)
                e[elevator].direction = -1;
        }
        // else if(at_floor==0 || at_floor==FLOORS-1) 
        //     direction*=-1;
        e[elevator].state=ELEVATOR_ARRIVED;
        printf("ELEVATOR ARRIVED\n");
    }
    pthread_mutex_unlock(&lock);
}