#define HOSTNAME_IDX 1
#define FILESYSTEM_IDX 2
#define NUM_PROCESS_IDX 3
#define PROGRAM_TO_RUN_IDX 4
#define PROGRAM_ARGS_IDX 5
#define MALLOC_ERROR "memory allocation failed"
#define SYS_CALL_ERROR "system call failed"
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <iostream>
#include <sys/stat.h>
#include <fstream>
#include <cstring>
#include <sys/unistd.h>
#include <string>
#define STACK 8192

using  std::string;

int num_of_args = 0;

void main_exit(const string& error, void* ptr) {
  std::cerr << "system error: " << error << std::endl;
  free(ptr);
  ptr = nullptr;
  exit(EXIT_FAILURE);
}

void child_exit(const string& error, char** ptr) {
  std::cerr << "system error: " << error << std::endl;
  free(ptr);
  ptr = nullptr;
  exit(EXIT_FAILURE);
}



void remove_files_and_dir (char* file_system){
  chroot (file_system);
  remove("/sys/fs/cgroup/pids/cgroup.procs");
  remove("/sys/fs/cgroup/pids/pids.max");
  remove("/sys/fs/cgroup/pids/notify_on_release");
  rmdir("/sys/fs/cgroup/pids");
  rmdir("/sys/fs/cgroup");
  rmdir("/sys/fs");
}

int child(void* argv) {
  char** args = (char**)argv;
  char* host_name = args[HOSTNAME_IDX];
  char* file_system = args[FILESYSTEM_IDX];
  char* num_process = args[NUM_PROCESS_IDX];
  char* program_to_run = args[PROGRAM_TO_RUN_IDX];

  char** program_args_char = (char**)malloc(sizeof(char*)*2);
  if (program_args_char == nullptr) {
    child_exit(MALLOC_ERROR, program_args_char);
  }

  program_args_char[0] = program_to_run;
  int j = 1;
  if (num_of_args > PROGRAM_ARGS_IDX){
      for (int i=PROGRAM_ARGS_IDX; i < num_of_args; i++) {
          program_args_char[j++] = args[i];
          char** tmp = (char**)realloc(program_args_char, (j+1)*sizeof(char*));
          if (!tmp) {
             child_exit(MALLOC_ERROR, program_args_char);
            }
        }
    }

  program_args_char[j] = (char*)0;

  if (sethostname(host_name, strlen(host_name)) == -1) {
    child_exit(SYS_CALL_ERROR, program_args_char);
  }

  if (chroot(file_system) == -1) {
    child_exit(SYS_CALL_ERROR, program_args_char);
  }

  int ret = chdir("/");
  if (ret == -1){
    child_exit(SYS_CALL_ERROR, program_args_char);
  }

  if (mkdir("/sys/fs", 0755)) {
    child_exit(SYS_CALL_ERROR, program_args_char);
    }

  if (mkdir("/sys/fs/cgroup", 0755)) {
    child_exit(SYS_CALL_ERROR, program_args_char);
    }

  if (mkdir("/sys/fs/cgroup/pids", 0755)) {
    child_exit(SYS_CALL_ERROR, program_args_char);
    }

  int child_pid = getpid();
  std::ofstream cgroup_procs;
  cgroup_procs.open("/sys/fs/cgroup/pids/cgroup.procs");
  chmod("/sys/fs/cgroup/pids/cgroup.procs", 0755);
  cgroup_procs << child_pid;
  cgroup_procs.close();

  std::ofstream pids_max;
  pids_max.open("/sys/fs/cgroup/pids/pids.max");
  chmod("/sys/fs/cgroup/pids/pids.max", 0755);
  pids_max << std::stoi(num_process);
  pids_max.close();
  std::ofstream notify_on_release;
  notify_on_release.open("/sys/fs/cgroup/pids/notify_on_release");
  chmod("/sys/fs/cgroup/pids/notify_on_release", 0755);
  notify_on_release << 1;
  notify_on_release.close();

  if (mount("proc","/proc", "proc", 0, 0) == -1) {
    child_exit(SYS_CALL_ERROR, program_args_char);
  }

  if (execvp(program_to_run, program_args_char) == -1) {
    child_exit(SYS_CALL_ERROR, program_args_char);
  }
  free(program_args_char);
  return 0;
}


int main(int argc, char* argv[]) {
  num_of_args = argc;
  void* stack = malloc(STACK);
  if (!stack) {
    std::cerr << "system error: " << MALLOC_ERROR << std::endl;
    exit(EXIT_FAILURE);
  }

  int child_pid = clone(child, (void*)((char*)stack + STACK), CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD,
                        (void*)argv);
  if (child_pid < 0) {
    main_exit(SYS_CALL_ERROR, stack);
  }

  wait(nullptr);

  std::string path(argv[FILESYSTEM_IDX]);
  path += "/proc";
  if (umount(path.c_str()) == -1) {
    main_exit(SYS_CALL_ERROR, stack);
  }

  free(stack);
  remove_files_and_dir (argv[FILESYSTEM_IDX]);
}