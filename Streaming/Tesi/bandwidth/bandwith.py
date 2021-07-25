import time
import psutil
import statistics
import matplotlib.pyplot as plt


def main():
    bandwidth = []
    tempo = []
    new_tempo = 0
    old_value = 0

    try:
        while True:
            new_value = psutil.net_io_counters().bytes_recv

            new_tempo += 0.5
            tempo.append(new_tempo)

            if old_value:
                bw = convert_to_mbit(new_value - old_value)
            else:
                bw = 0

            bandwidth.append(bw)

            old_value = new_value

            time.sleep(0.5)
    except KeyboardInterrupt:
        pass

    x = statistics.mean(bandwidth)
    print("mean: ", x)

    y = statistics.median(bandwidth)
    print("median: ", y)

    z = statistics.mode(bandwidth)
    print("mode: ", z)

    a = statistics.stdev(bandwidth)
    print("stdev: ", a)

    b = statistics.variance(bandwidth)
    print("variance: ", b)

    plt.plot(tempo, bandwidth)
    plt.xlabel('s')
    plt.ylabel('Mbps')
    plt.show()


def convert_to_mbit(value):
    return 8. * value / 1048576.


main()
