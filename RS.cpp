//
// Created by aj on 3/8/16.
//

#include "RS.h"

RS::RS(){
    busy = false;
    op = 0;
    lat = 0;
    result = 0;
    resultReady = false;
    Qj = 1;
    Qk = 1;
    Vj = 0;
    Vk = 0;
}
RS::RS(int OP, int RSoperandAvailable){
    busy = false;
    op = OP;
    lat = 0;
    result = 0;
    resultReady = false;
    Qj = RSoperandAvailable;
    Qk = RSoperandAvailable;
    Vj = 0;
    Vk = 0;

}