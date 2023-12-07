#!/usr/bin/env node

const WebSocket = require('ws');

const uri = 'ws://192.168.2.131/ws';

async function websocketClient() {
  for (let i = 0; i < 100000; i++) {
    const ws = new WebSocket(uri);

    if (i % 1000)
        console.log(`Count: ${i}`);

    ws.on('open', () => {
      console.log(`Connected to ${uri}`);
    });

    ws.on('message', (message) => {
      console.log(`Received message: ${message}`);
      ws.close();
    });

    ws.on('error', (error) => {
      console.error(`Error: ${error.message}`);
    });

    await new Promise((resolve) => {
      ws.on('close', () => {
        resolve();
      });
    });
  }
}

websocketClient();