import sys, psutil, time
pid = int( sys.argv[1] )
p = psutil.Process( pid )
print time.time() - p.create_time()

