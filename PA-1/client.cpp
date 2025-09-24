/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Ty Macaulay
	UIN: 634006348
	Date: 9/16/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool one_point = false;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				one_point = true;
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}
	pid_t pid = fork();
	if(pid == 0){
		//in child process
		//cout << "In child process. starting server" << endl;

		const char *args[] = {"./server", NULL};
		execvp("./server", const_cast<char* const*>(args));
	}else if(pid < 0){
		//Failed :(
		EXITONERROR("Fork failed");
	}
	//Parent (client) process
	//cout << "In parent process. Continueing client" << endl;
	sleep(1);
	
	FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	
	if(one_point){
		//./client -p x -t y -e z
		//Mode 1: single data point
		datamsg request(p, t, e);

		char buf[MAX_MESSAGE];
		memcpy(buf, &request, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg));
		double reply;
		chan.cread(&reply, sizeof(double));
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
	}else{
		//Mode 2: first 1000 data points.
		string filename = "received/x" + to_string(p) + ".csv";
		ofstream outfile(filename);

		for(int i = 0; i < 1000; i++){
			double time = 0.004*i;

			//ECG1
			datamsg request1(p, time, 1);
			char buf[MAX_MESSAGE];
			memcpy(buf, &request1, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			double ecg1;
			chan.cread(&ecg1, sizeof(double));	

			//ECG2
			datamsg request2(p, time, 2);
			memcpy(buf, &request2, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			double ecg2;
			chan.cread(&ecg2, sizeof(double));

			outfile << time << ", " << ecg1 << ", " << ecg2 << endl;
		}
		outfile.close();
		cout << "Finished writing to " << filename << endl;
	}
	
	// closing the channel    
	MESSAGE_TYPE m = QUIT_MSG;
	chan.cwrite(&m, sizeof(MESSAGE_TYPE));




	/* example data point request
	char buf[MAX_MESSAGE]; // 256
	datamsg x(1, 0.0, 1);
	
	memcpy(buf, &x, sizeof(datamsg));
	chan.cwrite(buf, sizeof(datamsg)); // question
	double reply;
	chan.cread(&reply, sizeof(double)); //answer
	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
	// sending a non-sense message, you need to change this
	filemsg fm(0, 0);
	string fname = "teslkansdlkjflasjdf.dat";
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);  // I want the file length;

	delete[] buf2;

	//quit MSG
*/
	
}
