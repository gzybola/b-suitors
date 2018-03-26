#include "blimit.hpp"

unsigned int bvalue(unsigned int method, unsigned long node_id) {

    switch (method) {

    case 0: return 1;

    default: switch (node_id) {

        case 0: return 2;

        case 1: return 2;

        default: return 1;

        }

    }

}
/*
unsigned int bvalue(unsigned int method, unsigned long node_id) {
switch (method) {
default: return (2 * node_id + method) % 10;
case 0: return 4;
case 1: return 7;
}
}

unsigned int bvalue(unsigned int method, unsigned long node_id) {
    switch (method) {
    default: return node_id % 42;
    case 0: return 4;
    case 1: return 7;
    }        
}
*/
/*
unsigned int bvalue(unsigned int method, unsigned long node_id) {
		switch(node_id) {
			case 0:
			case 3:
				return 2;
			case 8:
			case 6:
			case 1:
			case 13:
				return 1;
			case 9:
			case 2:
				return 2;
			case 7:
			case 14:
			case 11:
				return 4;
			default:
			return 3;
		}
}
*/
