#!/bin/bash

# 1. Path to the tester executable you built
TESTER_PATH="$HOME/Redis-Clone/redis-tester/redis-tester"

# 2. Set the project directories to the current folder
export CODECRAFTERS_SUBMISSION_DIR=$(pwd)
export CODECRAFTERS_REPOSITORY_DIR=$(pwd)

# 3. Choose which stage to test (Change "jm1" to other slugs as you progress)
export CODECRAFTERS_TEST_CASES_JSON='[{"slug": "la7", "tester_log_prefix": "stage-6", "title": "Stage 6: Implement SET & GET"}]'

# 4. Run the tester
$TESTER_PATH


#Command to run: 
# chmod +x run_test.sh
# ./run_test.sh

# Base Stages
# [
#     {"slug": "jm1", "tester_log_prefix": "stage-1", "title": "Stage 1: Bind to a port"},
#     {"slug": "rg2", "tester_log_prefix": "stage-2", "title": "Stage 2: Respond to PING"},
#     {"slug": "wy1", "tester_log_prefix": "stage-3", "title": "Stage 3: Respond to multiple PINGs"},
#     {"slug": "zu2", "tester_log_prefix": "stage-4", "title": "Stage 4: Handle concurrent clients"},
#     {"slug": "qq0", "tester_log_prefix": "stage-5", "title": "Stage 5: Implement ECHO"},
#     {"slug": "la7", "tester_log_prefix": "stage-6", "title": "Stage 6: Implement SET & GET"},
#     {"slug": "yz1", "tester_log_prefix": "stage-7", "title": "Stage 7: Expiry"}
# ]
# RDB Persistence Extension
# [
#     {"slug": "zg5", "tester_log_prefix": "rdb-1", "title": "RDB file config"},
#     {"slug": "jz6", "tester_log_prefix": "rdb-2", "title": "Read a key"},
#     {"slug": "gc6", "tester_log_prefix": "rdb-3", "title": "Read a string value"},
#     {"slug": "jw4", "tester_log_prefix": "rdb-4", "title": "Read multiple keys"},
#     {"slug": "dq3", "tester_log_prefix": "rdb-5", "title": "Read multiple string values"},
#     {"slug": "sm4", "tester_log_prefix": "rdb-6", "title": "Read value with expiry"}
# ]
# Replication Extension
# [
#     {"slug": "bw1", "tester_log_prefix": "repl-1", "title": "Configure listening port"},
#     {"slug": "ye5", "tester_log_prefix": "repl-2", "title": "The INFO command"},
#     {"slug": "hc6", "tester_log_prefix": "repl-3", "title": "The INFO command on a replica"},
#     {"slug": "xc1", "tester_log_prefix": "repl-4", "title": "Initial Replication ID and Offset"},
#     {"slug": "gl7", "tester_log_prefix": "repl-5", "title": "Send handshake (1/3)"},
#     {"slug": "eh4", "tester_log_prefix": "repl-6", "title": "Send handshake (2/3)"},
#     {"slug": "ju6", "tester_log_prefix": "repl-7", "title": "Send handshake (3/3)"},
#     {"slug": "fj0", "tester_log_prefix": "repl-8", "title": "Receive handshake (1/2)"},
#     {"slug": "vm3", "tester_log_prefix": "repl-9", "title": "Receive handshake (2/2)"},
#     {"slug": "cf8", "tester_log_prefix": "repl-10", "title": "Empty RDB Transfer"},
#     {"slug": "zn8", "tester_log_prefix": "repl-11", "title": "Single-replica propagation"},
#     {"slug": "hd5", "tester_log_prefix": "repl-12", "title": "Multi Replica Command Propagation"},
#     {"slug": "yg4", "tester_log_prefix": "repl-13", "title": "Command Processing"},
#     {"slug": "xv6", "tester_log_prefix": "repl-14", "title": "ACKs with no commands"},
#     {"slug": "yd3", "tester_log_prefix": "repl-15", "title": "ACKs with commands"},
#     {"slug": "my8", "tester_log_prefix": "repl-16", "title": "WAIT with no replicas"},
#     {"slug": "tu8", "tester_log_prefix": "repl-17", "title": "WAIT with no commands"},
#     {"slug": "na2", "tester_log_prefix": "repl-18", "title": "WAIT with multiple commands"}
# ]
# Streams Extension
# [
#     {"slug": "cc3", "tester_log_prefix": "stream-1", "title": "The TYPE command"},
#     {"slug": "cf6", "tester_log_prefix": "stream-2", "title": "Create a stream"},
#     {"slug": "hq8", "tester_log_prefix": "stream-3", "title": "Validating entry IDs"},
#     {"slug": "yh3", "tester_log_prefix": "stream-4", "title": "Partially auto-generated IDs"},
#     {"slug": "xu6", "tester_log_prefix": "stream-5", "title": "Fully auto-generated IDs"},
#     {"slug": "zx1", "tester_log_prefix": "stream-6", "title": "Query entries from stream"},
#     {"slug": "yp1", "tester_log_prefix": "stream-7", "title": "Query with -"},
#     {"slug": "fs1", "tester_log_prefix": "stream-8", "title": "Query with +"},
#     {"slug": "um0", "tester_log_prefix": "stream-9", "title": "Query single stream using XREAD"},
#     {"slug": "ru9", "tester_log_prefix": "stream-10", "title": "Query multiple streams using XREAD"},
#     {"slug": "bs1", "tester_log_prefix": "stream-11", "title": "Blocking reads"},
#     {"slug": "hw1", "tester_log_prefix": "stream-12", "title": "Blocking reads without timeout"},
#     {"slug": "xu1", "tester_log_prefix": "stream-13", "title": "Blocking reads using $"}
# ]
# Transactions Extension
# [
#     {"slug": "si4", "tester_log_prefix": "txn-1", "title": "INCR Command (1/3)"},
#     {"slug": "lz8", "tester_log_prefix": "txn-2", "title": "INCR Command (2/3)"},
#     {"slug": "mk1", "tester_log_prefix": "txn-3", "title": "INCR Command (3/3)"},
#     {"slug": "pn0", "tester_log_prefix": "txn-4", "title": "MULTI Command"},
#     {"slug": "lo4", "tester_log_prefix": "txn-5", "title": "EXEC Command"},
#     {"slug": "we1", "tester_log_prefix": "txn-6", "title": "Empty transaction"},
#     {"slug": "rs9", "tester_log_prefix": "txn-7", "title": "Queueing Commands"},
#     {"slug": "fy6", "tester_log_prefix": "txn-8", "title": "Executing a transaction"},
#     {"slug": "rl9", "tester_log_prefix": "txn-9", "title": "DISCARD Command"},
#     {"slug": "sg9", "tester_log_prefix": "txn-10", "title": "Failures within transactions"},
#     {"slug": "jf8", "tester_log_prefix": "txn-11", "title": "Multiple transactions"}
# ]
# Lists Extension
# [
#     {"slug": "mh6", "tester_log_prefix": "list-1", "title": "Create a list"},
#     {"slug": "tn7", "tester_log_prefix": "list-2", "title": "Append an element"},
#     {"slug": "lx4", "tester_log_prefix": "list-3", "title": "Append multiple elements"},
#     {"slug": "sf6", "tester_log_prefix": "list-4", "title": "List elements (positive indexes)"},
#     {"slug": "ri1", "tester_log_prefix": "list-5", "title": "List elements (negative indexes)"},
#     {"slug": "gu5", "tester_log_prefix": "list-6", "title": "Prepend elements"},
#     {"slug": "fv6", "tester_log_prefix": "list-7", "title": "Query list length"},
#     {"slug": "ef1", "tester_log_prefix": "list-8", "title": "Remove an element"},
#     {"slug": "jp1", "tester_log_prefix": "list-9", "title": "Remove multiple elements"},
#     {"slug": "ec3", "tester_log_prefix": "list-10", "title": "Blocking retrieval"},
#     {"slug": "xj7", "tester_log_prefix": "list-11", "title": "Blocking retrieval with timeout"}
# ]
# Pub/Sub Extension
# [
#     {"slug": "mx3", "tester_log_prefix": "pubsub-1", "title": "Subscribe to a channel"},
#     {"slug": "zc8", "tester_log_prefix": "pubsub-2", "title": "Subscribe to multiple channels"},
#     {"slug": "aw8", "tester_log_prefix": "pubsub-3", "title": "Enter subscribed mode"},
#     {"slug": "lf1", "tester_log_prefix": "pubsub-4", "title": "PING in subscribed mode"},
#     {"slug": "hf2", "tester_log_prefix": "pubsub-5", "title": "Publish a message"},
#     {"slug": "dn4", "tester_log_prefix": "pubsub-6", "title": "Deliver messages"},
#     {"slug": "ze9", "tester_log_prefix": "pubsub-7", "title": "Unsubscribe"}
# ]
# Sorted Sets Extension
# [
#     {"slug": "ct1", "tester_log_prefix": "zset-1", "title": "Create a sorted set"},
#     {"slug": "hf1", "tester_log_prefix": "zset-2", "title": "Add members"},
#     {"slug": "lg6", "tester_log_prefix": "zset-3", "title": "Retrieve member rank"},
#     {"slug": "ic1", "tester_log_prefix": "zset-4", "title": "List zset members"},
#     {"slug": "bj4", "tester_log_prefix": "zset-5", "title": "ZRANGE with negative indexes"},
#     {"slug": "kn4", "tester_log_prefix": "zset-6", "title": "Count zset members"},
#     {"slug": "gd7", "tester_log_prefix": "zset-7", "title": "Retrieve member score"},
#     {"slug": "sq7", "tester_log_prefix": "zset-8", "title": "Remove a member"}
# ]
# Geospatial Extension
# [
#     {"slug": "zt4", "tester_log_prefix": "geo-1", "title": "Respond to GEOADD"},
#     {"slug": "ck3", "tester_log_prefix": "geo-2", "title": "Validate coordinates"},
#     {"slug": "tn5", "tester_log_prefix": "geo-3", "title": "Store a location"},
#     {"slug": "cr3", "tester_log_prefix": "geo-4", "title": "Calculate location score"},
#     {"slug": "xg4", "tester_log_prefix": "geo-5", "title": "Respond to GEOPOS"},
#     {"slug": "hb5", "tester_log_prefix": "geo-6", "title": "Decode coordinates"},
#     {"slug": "ek6", "tester_log_prefix": "geo-7", "title": "Calculate distance"},
#     {"slug": "rm9", "tester_log_prefix": "geo-8", "title": "Search within radius"}
# ]
# Authentication & ACL Extension
# [
#     {"slug": "jn4", "tester_log_prefix": "auth-1", "title": "Respond to ACL WHOAMI"},
#     {"slug": "gx8", "tester_log_prefix": "auth-2", "title": "Respond to ACL GETUSER"},
#     {"slug": "ql6", "tester_log_prefix": "auth-3", "title": "The nopass flag"},
#     {"slug": "pl7", "tester_log_prefix": "auth-4", "title": "The passwords property"},
#     {"slug": "uv9", "tester_log_prefix": "auth-5", "title": "Setting default user password"},
#     {"slug": "hz3", "tester_log_prefix": "auth-6", "title": "The AUTH command"},
#     {"slug": "nm2", "tester_log_prefix": "auth-7", "title": "Enforce authentication"},
#     {"slug": "ws7", "tester_log_prefix": "auth-8", "title": "Authenticate using AUTH"}
# ]