TEMPLATE = subdirs
SUBDIRS += client server

client.file = $$PWD/client/client.pro
client.target = client

server.file = $$PWD/server/server.pro
server.target = server
