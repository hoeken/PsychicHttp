# ArduinoMongoose

[![Build Status](https://travis-ci.org/jeremypoulter/ArduinoMongoose.svg?branch=master)](https://travis-ci.org/jeremypoulter/ArduinoMongoose)

A wrapper for Mongoose to help build into Arduino framework.

# TODO:

* figure out how the original event handler flow worked
    * registers endpoints with mongoose
    * 
* update to the new style of single event handler callback

* read through again to understand the library and how the user_data passing stuff works
* figure out if we can eliminate the user_connection_data requirement
* figure out the sntp stuff: tv, user_data (might be easiest test case)
* mongoose.c changes:
    * a bunch of irrelevant #includes
    * some mbedtls certificate parsing changes
    * a bunch of user_data calls