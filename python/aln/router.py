from threading import Thread, Lock

class Router:
    def __init__(self, address):
        self.address = address
        self.lock = Lock()
        self.channels = []
	    self.serviceMap = {}     # map[address]callback registered local node packet handlers
	    self.contextMap = {}     # serviceHandlerMap
	    self.remoteNodeMap = {}  # RemoteNodeMap // map[address][]RemoteNodes
	    self.serviceLoadMap = {} # ServiceLoadMap


    def select_service(self, serviceID):
        pass
    
    def send(self, packet):
        pass

    def register_context_handler(self, callback):
        pass

    def release_context(self, ctxID):
        pass

    def register_service(self, serviceID, handler):
        pass

    def export_routes(self):
        pass

    def export_services(self);
        pass

    def share_net_state(self):
        