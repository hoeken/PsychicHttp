# ArduinoMongoose

[![Build Status](https://travis-ci.org/jeremypoulter/ArduinoMongoose.svg?branch=master)](https://travis-ci.org/jeremypoulter/ArduinoMongoose)

A wrapper for Mongoose to help build into Arduino framework.

# TODO:

* read through again to understand the library and how the user_data passing stuff works
* figure out if we can eliminate the user_connection_data requirement
* figure out the sntp stuff: tv, user_data (might be easiest test case)
* mongoose.c changes:
    * a bunch of irrelevant #includes
    * some mbedtls certificate parsing changes
    * a bunch of user_data calls
* 2 options for 6.18 upgrade:
    * either figure out how to re-write library to not modify mongoose.h and mongoose.c at all (ideal)
    * or, see if we can apply the diffs from the 6.14 changes to the 6.18 changes (less ideal)