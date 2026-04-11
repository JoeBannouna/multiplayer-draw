# Setting up
You need SDL2. On Ubuntu, installing it through apt goes like this:
```bash
sudo apt install libsdl2-dev
```

### NOT REQUIRED
The project is not yet complex enough to need these extensions:
```
libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
```
However, when the base networking model is complete the focus will shift on the art/drawing features and they will likely be needed.

# Compilation and Running
Makefile is faster for running quickly and debugging
```bash
make server
```
```bash
make client
```

For running
```bash
./build/server
```
```bash
./build/client localhost # Or replace localhost with the target IP
```

# CMake Build
If you want CMake instead:
```bash
cd build && cmake .. && make && cd ..
```
This will make both the client and server executables.

# Security notice
This is just a practice project.
Due to the low-level nature of C, I advice you not to trust this code in a production environment, or run it at your own risk.

# Roadmap
- [x] Basic Client-Server Communication
- [x] Event Based Server Architecture with Thread-Per-Client approach
- [x] Simultaneous bi-directional communication on client side
- [x] Joining the server and keeping track of connected players on Client and Server
- [x] New player connections updates streamed to current players in the server
- [x] Real-time location updates
- [ ] Player disconnection updates streamed to other players
- [ ] Client drawing functionality
- [ ] Canvas zoom/pan features (grid approach)
- [ ] Drawing actions streamed to other players
- [ ] Storing art files in an efficient format on the server and client
- [ ] More complex UI layouts 
  - [ ] accepting text input
  - [ ] buttons
  - [ ] modes
  - [ ] layers?
- [ ] UI Chatting functionality
- [ ] Cross-platform releases
