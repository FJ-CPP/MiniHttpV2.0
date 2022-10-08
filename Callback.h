#pragma once
#include <iostream>
#include <arpa/inet.h>
#include <strings.h>
#include "Reactor.h"
#include "Protocol.h"

int ReceiverHelper(int fd, std::string &rbuffer);

void Receiver(Event &event);

int SenderHelper(int fd, std::string &wbuffer);

void Sender(Event &event);

void Errorer(Event &event);

void Accepter(Event &event);
