import subprocess
import sys
import time

num_tests = "100"  #string to pass to api_test.exe

#arguments to run.py is csv file for which SNRs to look up BERs to run

if __name__ == "__main__":
    #run.py input_filename output_filename
    if(len(sys.argv) != 3):
        print("Needs arguments: input_filename output_filename")
        exit(0)

    csv_contents = open(sys.argv[1], "r").read().split("\n")
    if(csv_contents[-1] == ''):
        csv_contents = csv_contents[:-1]

    csv_contents = [[a for a in line.split(",")] for line in csv_contents]

    processes = []
    BERs = []
    SNRs = []
    outputs = []

    num_waiting = 0

    #run the processes 4 at a time
    for i in range(0, len(csv_contents)-4, 4):
        BER0 = csv_contents[i][1]
        BER1 = csv_contents[i+1][1]
        BER2 = csv_contents[i+2][1]
        BER3 = csv_contents[i+3][1]
        BERs += [BER0, BER1, BER2, BER3]
        SNRs += [csv_contents[i][0], csv_contents[i+1][0], csv_contents[i+2][0], csv_contents[i+3][0]]

        processes.append(subprocess.Popen(["api_test.exe", num_tests, BER0], stdout=subprocess.PIPE))
        processes.append(subprocess.Popen(["api_test.exe", num_tests, BER1], stdout=subprocess.PIPE))
        processes.append(subprocess.Popen(["api_test.exe", num_tests, BER2], stdout=subprocess.PIPE))
        processes.append(subprocess.Popen(["api_test.exe", num_tests, BER3], stdout=subprocess.PIPE))
        try:
            outputs.append(float(processes[-4].communicate()[0]))
        except ValueError:
            outputs.append(-1.0)
        try:
            outputs.append(float(processes[-3].communicate()[0]))
        except ValueError:
            outputs.append(-1.0)
        try:
            outputs.append(float(processes[-2].communicate()[0]))
        except ValueError:
            outputs.append(-1.0)
        try:
            outputs.append(float(processes[-1].communicate()[0]))
        except ValueError:
            outputs.append(-1.0)

        print("%i / %i done     " % (i, len(csv_contents)), end="\r")

    #run any remaining processes
    for i in range((len(csv_contents)-4) // 4 * 4, len(csv_contents)):
        BER = csv_contents[i][1]
        BERs.append(BER)
        SNRs.append(csv_contents[i][0])

        processes.append(subprocess.Popen(["api_test.exe", num_tests, BER], stdout=subprocess.PIPE))
        try:
            outputs.append(float(processes[-1].communicate()[0]))
        except ValueError:
            outputs.append(-1.0)
        print("%i / %i done     " % (i, len(csv_contents)), end="\r")
    print("%i / %i done     " % (len(csv_contents), len(csv_contents)))


    f = open(sys.argv[2], "w")
    f.write("SNR,BER,message pass rate\n")
    for i in range(len(SNRs)):
        try:
            f.write("%s,%s,%s\n" % (SNRs[i], BERs[i], outputs[i]))
        except TypeError:
            print(SNRs[i])
            print(BERs[i])
            print(outputs[i])
    f.close()
