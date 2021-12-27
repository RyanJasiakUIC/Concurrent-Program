#include "hw6.h"
#include <stdio.h>
#include<pthread.h>

typedef struct {
    pthread_mutex_t lock;
    // pthread_mutex_t enter_lock;
    pthread_cond_t passenger_has_exited;
    pthread_cond_t passenger_has_entered;
    int is_occupied;
    enum {yes = 1, no = 0} condition;
    enum {yes2 = 1, no2 = 0} condition2;

    int current_floor;
    int direction;
    int occupancy;
    enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;

    int pas_req[PASSENGERS];
    int next_requested_index;
    int num_requests;

    int passengers_riding[PASSENGERS];
    int current_passenger;
} ELEV;

int whos_on_what_elevator[PASSENGERS];
int assignment_index = 0;

ELEV e[ELEVATORS];


void scheduler_init() {
    for (int j = 0; j < ELEVATORS; j++) {
        e[j].current_floor=0;		
        e[j].direction=-1;
        e[j].occupancy=0;
        e[j].condition = no;
        e[j].condition2 = no2;
        e[j].state = ELEVATOR_ARRIVED;
        e[j].num_requests = 0;
        e[j].next_requested_index = 0;
        e[j].current_passenger = -1;
        pthread_mutex_init(&e[j].lock,0);
        pthread_cond_init(&e[j].passenger_has_entered, 0);
        pthread_cond_init(&e[j].passenger_has_exited, 0);
        for (int i = 0; i < PASSENGERS; i++) {
            e[j].pas_req[i] = -1;
            e[j].passengers_riding[i] = -1;
        }
    }
    for (int j = 0; j < PASSENGERS; j++)
        whos_on_what_elevator[j] = -1;
}

int add_request(int from, int passenger) {
    if (assignment_index == ELEVATORS) assignment_index = 0;
    // printf("assigning passenger %d to elevator %d\n", passenger, assignment_index);
    whos_on_what_elevator[passenger] = assignment_index++;
    e[whos_on_what_elevator[passenger]].pas_req[passenger] = from;
    return whos_on_what_elevator[passenger];
}

int get_next_request(int elev) {
    ELEV *_e = &e[elev];
    if (_e->next_requested_index >= PASSENGERS)
        _e->next_requested_index = 0;
    int n = 0;
    while (_e->pas_req[_e->next_requested_index] == -1) {
        _e->next_requested_index++;
        if (_e->next_requested_index >= PASSENGERS)
            _e->next_requested_index = 0;
        if (++n > PASSENGERS)
            return 0;
    }
    return _e->pas_req[_e->next_requested_index++];
}

void remove_request(int passenger) {
    e[whos_on_what_elevator[passenger]].pas_req[passenger] = -1;
}

void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{	
    int elev = add_request(from_floor, passenger);
    e[elev].num_requests++;
    // if (num_requests == 1) current_floor = from_floor;
    // printf("curent floor: %d\n", current_floor);
    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    int x = 0;
    while(waiting) {
        pthread_mutex_lock(&e[elev].lock);
        ELEV *_e = &e[elev];
        if(_e->current_floor == from_floor && _e->state == ELEVATOR_OPEN && _e->occupancy==0) {
            enter(passenger, elev);
            _e->current_passenger = passenger;
            _e->passengers_riding[_e->current_passenger] = to_floor;
            _e->occupancy++;
            waiting=0;
            _e->condition = yes;
            pthread_cond_signal(&_e->passenger_has_entered);
        }
        // if (x++ % 1000 == 0)
        //     printf("curent floor: %d\n", current_floor);
        if (_e->current_floor == 0) {
            // if (x++ < 30)
            // printf("curent floor: %d\n", current_floor);
            _e->condition = yes;
            pthread_cond_signal(&_e->passenger_has_entered);
        }
        pthread_mutex_unlock(&e[elev].lock);

        // pthread_mutex_unlock(&lock);
        // condition = yes;
        // pthread_cond_signal(&passenger_has_entered);
        // pthread_cond_signal(&passenger_has_entered);
    }

    // wait for the elevator at our destination floor, then get out
    int riding=1;
            // printf("WHAT\n");
    while(riding) {
        pthread_mutex_lock(&e[elev].lock);
        // printf("What teh heck!\n");
        ELEV *_e = &e[elev];
            // printf("WHAT\n");
        if(_e->current_floor == to_floor && _e->state == ELEVATOR_OPEN) {
            // printf("***\ncurren_floor: %d\nto_floor: %d\n***\n", current_floor, to_floor);
            exit(passenger, elev);
            _e->occupancy--;
            _e->current_passenger = -1;
            _e->num_requests--;
            remove_request(passenger);
            // printf("occupancy: %d\n", occupancy);
            riding=0;
        }
        pthread_mutex_unlock(&e[elev].lock);
        _e->condition2 = yes2;
        pthread_cond_signal(&e[elev].passenger_has_exited);
    }
}

int passenger_is_waiting_at_floor(int elevator, int floor)  {
    for (int i = 0; i < PASSENGERS; i++) {
        if (e[elevator].pas_req[i] == floor)
            return 1;
    }
    return 0;
}

void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)) {
    // if(elevator!=0) return;
    if(e[elevator].num_requests==0) return;
    // printf("read\n!!\n");
    ELEV *_e = &e[elevator];
    pthread_mutex_lock(&_e->lock);

    if(_e->state == ELEVATOR_ARRIVED) {
        // printf("BRUH WHY\n");
        door_open(elevator);
        _e->state=ELEVATOR_OPEN;
    }
    else if(_e->state == ELEVATOR_OPEN) {
        if (_e->occupancy == 0 && _e->num_requests > 0) {
            _e->condition = no;
            // printf("no occupancy, num_requests: %d!!!!!!!!\n", _e->num_requests);
            while (_e->condition == no && _e->num_requests > 0 && passenger_is_waiting_at_floor(elevator, at_floor))
                pthread_cond_wait(&_e->passenger_has_entered, &_e->lock);
        } else if (_e->current_passenger != -1 && _e->passengers_riding[_e->current_passenger] == at_floor) {
            _e->condition2 = no2;
            while (_e->condition2 == no2)
                pthread_cond_wait(&_e->passenger_has_exited, &_e->lock);
        }
        door_close(elevator);
        _e->state=ELEVATOR_CLOSED;
    }
    else {
        int old_direction = _e->direction;
        if (_e->occupancy != 0) {
            int to_floor = _e->passengers_riding[_e->current_passenger];
            _e->direction = to_floor-at_floor;
        } 
        else if (_e->num_requests > 0) {
            int to_floor = get_next_request(elevator);
            // printf("NEXT FLOOR: %d\n", to_floor);
            _e->direction = to_floor-at_floor;
        }
        else if (at_floor<=0 || at_floor>=FLOORS-1) {
            _e->direction *=- 1;
            // printf("direction: %d\n", _e->direction);
        }
        if (at_floor + _e->direction < FLOORS && at_floor+_e->direction > -1)
            move_direction(elevator,_e->direction);
        _e->current_floor=at_floor+_e->direction;
        if (_e->direction != 1 && _e->direction != -1) {
            _e->direction = old_direction;
            if (at_floor == 0)
                _e->direction = 1;
            else if (at_floor >= FLOORS-1)
                _e->direction = -1;
        }
        _e->state=ELEVATOR_ARRIVED;
        // printf("ELEVATOR ARRIVED\n");
    }
    pthread_mutex_unlock(&_e->lock);
}