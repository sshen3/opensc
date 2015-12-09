all:agent firewall controller demo_video demo_clear vlc_player video_relay
agent:interface.cpp func.cpp agent.cpp
	g++ -o agent interface.cpp func.cpp agent.cpp -ljson-c
firewall:interface.cpp func.cpp firewall_agent.cpp firewall.cpp
	g++ -o firewall interface.cpp func.cpp firewall_agent.cpp firewall.cpp -ljson-c
controller:interface.cpp func.cpp controller.cpp
	g++ -o controller func.cpp interface.cpp controller.cpp -ljson-c
demo_video:demo_video.cpp opensc.cpp
	g++ -o demo_video opensc.cpp demo_video.cpp
demo_clear:demo_clear.cpp opensc.cpp
	g++ -o demo_clear opensc.cpp demo_clear.cpp
vlc_player:vlc_player.cpp func.cpp
	g++ -o vlc_player vlc_player.cpp func.cpp
video_relay:video_relay.cpp
	g++ -o video_relay video_relay.cpp
clean:
	rm -f agent
	rm -f firewall
	rm -f controller
	rm -f demo_video
