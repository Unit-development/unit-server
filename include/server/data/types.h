//
// Created by Kirill Zhukov on 20.11.2023.
//

#ifndef TYPES_H
#define TYPES_H

namespace unit::server::request {
        enum type : int {
            GET = 0,
            POST = 1,
            HEAD = 2,
            PUT = 3,
            DELETE = 4,
            CONNECT = 5,
            OPTIONS = 6,
            TRACE = 7,
            PATCH = 8
        };
}; // unit::server::request

#endif //TYPES_H
