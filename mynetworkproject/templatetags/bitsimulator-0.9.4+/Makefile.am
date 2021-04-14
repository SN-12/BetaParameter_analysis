bin_PROGRAMS = bitsimulator visualtracer
bitsimulator_SOURCES = src/bitsimulator.cpp src/events.cpp src/events.h src/eventtypes.h src/node.cpp src/node.h src/output.cpp src/output.h src/packet.cpp src/packet.h src/scheduler.cpp src/scheduler.h src/utils.cpp src/utils.h src/world.cpp src/world.h \
	src/agents/application-agent.cpp src/agents/application-agent.h src/agents/backoff-deviation-routing-agent.cpp src/agents/backoff-deviation-routing-agent.h src/agents/backoff-flooding-routing-agent.cpp src/agents/backoff-flooding-routing-agent.h src/agents/backoff-flooding-ring-routing-agent.cpp src/agents/backoff-flooding-ring-routing-agent.h src/agents/cbr-application-agent.cpp src/agents/cbr-application-agent.h src/agents/confidence-routing-agent.cpp src/agents/confidence-routing-agent.h src/agents/datasink-application-agent.cpp src/agents/datasink-application-agent.h src/agents/deden-agent.cpp src/agents/deden-agent.h src/agents/gateway-server-agent.cpp src/agents/gateway-server-agent.h src/agents/hcd-routing-agent.cpp src/agents/hcd-routing-agent.h src/agents/incident-observer-agent.cpp src/agents/incident-observer-agent.h src/agents/manual-routing-agent.cpp src/agents/manual-routing-agent.h src/agents/no-routing-agent.cpp src/agents/no-routing-agent.h src/agents/proba-flooding-routing-agent.cpp src/agents/proba-flooding-routing-agent.h src/agents/proba-flooding-ring-routing-agent.cpp src/agents/proba-flooding-ring-routing-agent.h src/agents/pure-flooding-routing-agent.cpp src/agents/pure-flooding-routing-agent.h src/agents/pure-flooding-ring-routing-agent.h src/agents/pure-flooding-ring-routing-agent.cpp src/agents/routing-agent.cpp src/agents/routing-agent.h src/agents/server-application-agent.cpp src/agents/server-application-agent.h src/agents/slr-backoff-routing-agent.cpp src/agents/slr-backoff-routing-agent.h src/agents/slr-backoff-routing-agent3.cpp src/agents/slr-backoff-routing-agent3.h src/agents/slr-routing-agent.cpp src/agents/slr-routing-agent.h src/agents/slr-deviation-routing-agent.cpp src/agents/slr-deviation-routing-agent.h src/agents/slr-ring-routing-agent.cpp src/agents/slr-ring-routing-agent.h
visualtracer_SOURCES = src/output.cpp src/renderer.cpp src/renderer.h src/output.h src/utils.cpp src/visualtracer.cpp

TESTS = tests/test1.sh
# uniform_int_distribution is implemented differently by compilers, and this test works only if the compiler is gcc
if !USE_GCC
XFAIL_TESTS = tests/test1.sh
endif

EXTRA_DIST = tests/test1.sh tests/expected-events.log tests/scenario.xml

AM_CPPFLAGS = -Wall -Wextra -std=c++11 -march=native $(freetype2_CFLAGS)

#visualtracer_CPPFLAGS = $(freetype2_CFLAGS) $(GL_CFLAGS)
visualtracer_LDADD = $(freetype2_LIBS) $(GL_LIBS)
