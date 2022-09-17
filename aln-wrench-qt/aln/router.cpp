#include "router.h"

Router::Router(QString address) {
    if (address.length() > 0) {
        mAddress = address;
    }
}
