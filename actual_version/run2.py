import subprocess
import sys
import time

max_num_tests = "50000"  #string to pass to api_test.exe
max_num_errors = "500"
num_cores = 4
starting_SNR = -2.5

#arguments to run.py is csv file for which SNRs to look up BERs to run

if __name__ == "__main__":
    #run.py input_filename output_filename t
    if(len(sys.argv) != 4):
        print("Needs arguments: input_filename output_filename t")
        exit(0)

    csv_contents = open(sys.argv[1], "r").read().split("\n")
    if(csv_contents[-1] == ''):
        csv_contents = csv_contents[:-1]

    csv_contents = [[a for a in line.split(",")] for line in csv_contents]

    csv_contents2 = []

    for i in range(0, len(csv_contents), 50):
        csv_contents2.append(csv_contents[i])
    csv_contents = [line for line in csv_contents2 if float(line[0]) >= starting_SNR]

    t = sys.argv[3]

    processes = []
    BERs = []
    SNRs = []
    outputs = []

    num_waiting = 0

    #run the processes 4 at a time
    premature_exit = 0
    for i in range(0, len(csv_contents)-num_cores, num_cores):
        BER = []
        SNR = []
        for j in range(num_cores):
            BER.append(csv_contents[i+j][1])
            SNR.append(csv_contents[i+j][0])
        BERs += BER
        SNRs += SNR

        for j in range(num_cores):
            processes.append(subprocess.Popen(["api_test.exe", "sim", max_num_tests, max_num_errors, BER[j], t], stdout=subprocess.PIPE))
        for j in range(num_cores):
            try:
                outputs.append(float(processes[-num_cores+j].communicate()[0]))
            except ValueError:
                outputs.append(-1.0)

        print("%i / %i done     " % (i, len(csv_contents)), end="\r")
        if(outputs[-1] == 1.0 or outputs[-2] == 1.0 or outputs[3] == 1.0 or outputs[-4] == 1.0):
            print("exiting: 0 found.")
            premature_exit = 1
            break

    #run any remaining processes
    if(not premature_exit):
        for i in range((len(csv_contents)-num_cores) // num_cores * num_cores, len(csv_contents)):
            BER = csv_contents[i][1]
            BERs.append(BER)
            SNRs.append(csv_contents[i][0])

            processes.append(subprocess.Popen(["api_test.exe", "sim", max_num_tests, max_num_errors, BER, t], stdout=subprocess.PIPE))
            try:
                outputs.append(float(processes[-1].communicate()[0]))
            except ValueError:
                outputs.append(-1.0)
            print("%i / %i done     " % (i, len(csv_contents)), end="\r")
        print("%i / %i done     " % (len(csv_contents), len(csv_contents)))

    try:
        f = open(sys.argv[2], "w")
    except PermissionError:
        tries = 2
        while 1:
            try:
                f = open(sys.argv[2] + str(tries), "w")
                break
            except PermissionError:
                tries += 1
            if(tries > 100):
                break

    f.write("SNR,BER,message fail rate\n")
    for i in range(len(SNRs)):
        try:
            f.write("%s,%s,%f\n" % (SNRs[i], BERs[i], 1 - float(outputs[i])))
        except TypeError:
            print(SNRs[i])
            print(BERs[i])
            print(outputs[i])
    f.close()
