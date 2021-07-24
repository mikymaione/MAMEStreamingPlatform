import time
import psutil
import matplotlib.pyplot as plt


def main():
    bandwith = []
    tempo = []
    new_tempo = 0
    old_value = 0

    try:
        while True:
            new_value = psutil.net_io_counters().bytes_recv
            # psutil.net_io_counters().bytes_recv
            # psutil.net_io_counters().bytes_sent

            new_tempo += 0.5
            tempo.append(new_tempo)

            if old_value:
                bw = convert_to_mbit(new_value - old_value)
            else:
                bw = 0

            bandwith.append(bw)

            old_value = new_value

            time.sleep(0.5)
    except KeyboardInterrupt:
        pass

    plt.plot(tempo, bandwith)
    plt.xlabel('s')
    plt.ylabel('Mbps')
    plt.show()


def convert_to_mbit(value):
    return 8. * value / 1048576.


main()
