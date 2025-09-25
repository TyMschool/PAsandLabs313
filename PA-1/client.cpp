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
#include <sys/wait.h>

using namespace std;

void requestFiles(FIFORequestChannel& chan, const string& filename, int buffer_capacity){
	filemsg size_request(0, 0);  // offset=0, length=0 means "give me file size"
    
    // Build message: filemsg + filename
    int len = sizeof(filemsg) + filename.size() + 1;
    char* buf = new char[len];
    memcpy(buf, &size_request, sizeof(filemsg));
    strcpy(buf + sizeof(filemsg), filename.c_str());
    
    // Send request
    chan.cwrite(buf, len);
    
    // Receive file size
    __int64_t file_size;
    chan.cread(&file_size, sizeof(__int64_t));
    
    cout << "File size: " << file_size << " bytes" << endl;
    delete[] buf;
    
    if (file_size <= 0) {
        cout << "File not found or empty" << endl;
        return;
    }

	// Create output file
    string output_path = "received/" + filename; 
    ofstream outfile(output_path, ios::binary);
    
    __int64_t bytes_received = 0;
    
    while (bytes_received < file_size) {
		__int64_t remaining = file_size - bytes_received;
        int request_size = (remaining > buffer_capacity) ? buffer_capacity : (int)remaining;
        
        // Request this chunk
        filemsg chunk_request(bytes_received, request_size);
        
        // Build and send message
        len = sizeof(filemsg) + filename.size() + 1;
        buf = new char[len];
        memcpy(buf, &chunk_request, sizeof(filemsg));
        strcpy(buf + sizeof(filemsg), filename.c_str());
        chan.cwrite(buf, len);
        
        // Receive chunk data
        char* response = new char[request_size];
        int actual_bytes = chan.cread(response, request_size);
        
        // Write to file
        outfile.write(response, actual_bytes);
        bytes_received += actual_bytes;
        
        // Cleanup
        delete[] buf;
        delete[] response;
    }
    
    outfile.close();
	cout << "file saved to " << output_path << endl;
}

int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	int buffer_capacity = MAX_MESSAGE;
	bool one_point = false;
	bool request_files = false;
	bool new_channel = false;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
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
				request_files = true;
				break;
			case 'm':
				buffer_capacity = atoi(optarg);
				break;
			case 'c':
				new_channel = true;
				break;
		}
	}
	pid_t pid = fork();
	if(pid == 0){
		//in child process
		//cout << "In child process. starting server" << endl;
		if(buffer_capacity != MAX_MESSAGE){
			string arg_buffer = to_string(buffer_capacity);
			//execute server with the new buffer_capacity
			const char *args[] = {"./server", "-m", arg_buffer.c_str(), NULL};
			execvp("./server", const_cast<char* const*>(args));
		} else {
			//default capacity
			const char *args[] = {"./server", NULL};
			execvp("./server", const_cast<char* const*>(args));
		}
		
	}else if(pid < 0){
		//Failed :(
		EXITONERROR("Fork failed");
	}
	//Parent (client) process
	//cout << "In parent process. Continueing client" << endl;
	sleep(1);
	
	FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel* active_channel = &chan;
	vector<FIFORequestChannel*> cleanup;

	if(new_channel){
		
		MESSAGE_TYPE msg = NEWCHANNEL_MSG;
		chan.cwrite(&msg, sizeof(MESSAGE_TYPE));

		char new_channel_name[30];
		chan.cread(new_channel_name, sizeof(new_channel_name));

		string channel_name = string (new_channel_name);
		cout << "Server created new channel - " << channel_name << endl;

		FIFORequestChannel* data_channel = new FIFORequestChannel(channel_name, FIFORequestChannel::CLIENT_SIDE);

		cleanup.push_back(data_channel);
		active_channel = data_channel;
	}
	
	if(request_files){
		//./client -f x
		//Mode 3: Requesting files
		string copy_cmd = "cp " + filename + " BIMDC/ 2>/dev/null || true";
    	system(copy_cmd.c_str());
		
		requestFiles(*active_channel, filename, buffer_capacity);

	} else if(one_point){
		//./client -p x -t y -e z
		//Mode 1: single data point
		datamsg request(p, t, e);

		char buf[MAX_MESSAGE];
		memcpy(buf, &request, sizeof(datamsg));
		active_channel->cwrite(buf, sizeof(datamsg));
		double reply;
		active_channel->cread(&reply, sizeof(double));
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
	} else {
		//./client -p x
		//Mode 2: first 1000 data points.
		string filename = "received/x" + to_string(p) + ".csv";   /// OR HERE?? x or no x?
		ofstream outfile(filename);

		for(int i = 0; i < 1000; i++){
			double time = 0.004*i;

			//ECG1
			datamsg request1(p, time, 1);
			char buf[MAX_MESSAGE];
			memcpy(buf, &request1, sizeof(datamsg));
			active_channel->cwrite(buf, sizeof(datamsg));
			double ecg1;
			active_channel->cread(&ecg1, sizeof(double));	

			//ECG2
			datamsg request2(p, time, 2);
			memcpy(buf, &request2, sizeof(datamsg));
			active_channel->cwrite(buf, sizeof(datamsg));
			double ecg2;
			active_channel->cread(&ecg2, sizeof(double));

			outfile << time << ", " << ecg1 << ", " << ecg2 << endl;
		}
		outfile.close();
		cout << "Finished writing to " << filename << endl;
	}
	
	// closing the channels  
	for(FIFORequestChannel* channel : cleanup){
		if(channel != nullptr){
			cout << "closing channel - " << channel->name() << endl;
			MESSAGE_TYPE quit = QUIT_MSG;
			channel->cwrite(&quit, sizeof(MESSAGE_TYPE));

			delete channel;
			channel = nullptr;
		}
	}

	MESSAGE_TYPE quit = QUIT_MSG;
	chan.cwrite(&quit, sizeof(MESSAGE_TYPE));

	int status;
	wait(&status);
	cout << "server gone: status " << status << endl;

	return 0;
}
