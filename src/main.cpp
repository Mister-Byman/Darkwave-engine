#include "engine/Engine.h"

int main(int argc, char** argv) {
	Engine engine;
	if (!engine.init()) return 1;
	engine.run();
	engine.shutdown();
	return 0;
}
