
# Avsar (अवसर) - Smart Seat Booking & Waiting List System

**Har Slot Ka Sahi Avsar!**

Avsar is a C++ booking system for college labs, sports facilities and shared equipment. It manages seat allocation, keeps booking records, and runs cancellation waiting lists. When someone cancels, the freed seat goes automatically to the next person in line.

## Problem it solves
Labs, courts and halls often sit partly empty after no-shows and late cancellations, while other students wait with no way to get in. Avsar reuses those seats and keeps the process fair.

## Features
- User and admin login (hashed passwords)
- Seat-level booking with a visual seat map, or auto-assign
- Conflict detection (no double booking at the same time)
- First-come-first-served cancellation waiting list
- Auto seat recovery: a freed seat is allocated to the first waiting user
- Smart alternative slot suggestions when a slot is full
- Booking modification, cancellation and no-show handling
- Booking history and records saved to files
- Notifications for confirmations and waitlist promotions
- Admin dashboard with seat utilization report

## ADSA concepts used
| Concept | Where it is used |
|---|---|
| Queue | Cancellation waiting list for each slot |
| Min-Heap | Lowest free seat, and earliest alternative slots |
| Hash Table | O(1) booking ID lookup (custom chained table) |
| Sorting | Resource utilization ranking |
| Searching | Conflict checks and slot scanning |

## How to run
```
g++ -std=c++17 avsar.cpp -o avsar
./avsar
```
Admin login: `admin` / `admin123`

## Demo tip
Use the Projector Kit (1 seat): book it with one user, try again with a second user, join the waiting list, then cancel with the first user.
