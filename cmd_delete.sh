curl http://localhost:8080/wm/staticflowpusher/list/00:02:a0:1d:48:84:4f:00/json
curl -X DELETE -d '{"switch": "00:02:a0:1d:48:84:4f:00", "name":"flow-mod-1", "eth_type":"0x0800", "ipv4_src":"10.1.1.5", "ipv4_dst":"10.1.1.4", "cookie":"0", "priority":"32768", "in_port":"3","active":"true", "actions":"output=2"}' http://localhost:8080/wm/staticflowpusher/json
curl http://localhost:8080/wm/staticflowpusher/list/00:02:a0:1d:48:84:4f:00/json
