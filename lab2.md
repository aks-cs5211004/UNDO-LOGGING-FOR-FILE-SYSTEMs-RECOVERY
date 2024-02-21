# COL331 Lab 2 (Part-A): Undo Logs in xv6
In `p12-log`, you have studied the implementation of write-ahead logging (WAL) with redo logs in `xv6`, which helped ensure atomicity with respect to crashes, and hence crash consistency.

In this lab, you will explore another method of write ahead logging, which uses _undo logs_ instead of redo logs. 

## Undo Logs
The idea of undo logs is as follows: before we write a block to the disk, we first log the old value of the block, and then write to the disk, and finally consider the transaction to be committed. In case of a crash, we simply check the log for uncommitted transactions, and "undo" their effect. 

We can describe this procedure more formally using the following rules:

### Undo Log Rules 
To write new values `n1`, `n2` to blocks B1, B2:

- Write old values `o1`, `o2` into the log
- Start the transaction which writes log header block describing the block numbers of `o1` and `o2`
- Install the transaction: flush the values `n1` and `n2` to B1, B2 on disk 
- Commit the transaction by clearing the log

### Undo Log Recovery

Read the log header block and write the blocks in the log described in the log header.

We may summarize the difference between redo and undo logging via the following table: 

| Status of block at time of crash | Redo Log | Undo Log |
| -------- | --------| ------- |
| Committed & written to disk  | Nothing to do | Nothing to do |
| Committed but not written     | Redo by writing the new value in log to disk | Not possible |
| Written but not committed     | Not possible | Undo by writing the old value in log to disk |
| Not written and not committed     | Nothing to do | Nothing to do |

### Food for Thought
Try to compare performance and durability of redo and undo logs. Which method of logging would be more efficient for an operating system? And which method might lose more data if we assume rest of the system design is identical. 

## Implementation
The implementation of undo logging will involve changes to `log.c` and `bio.c`. You are encouraged to read and understand `p12-log.md`, as well as how the buffer cache works. 

Note that in undo logs, we write the old value of a block to the log which helps us undo the operation, which may require an additional read from the disk.

To prevent this performance hit, we cache the old value of disk whenever we perform a write. For this purpose, a new function `bread_wr` has been defined in `bio.c`, which is now called in `fs.c` wherever `bread` was called with the intention of *modifying the block*. You must provide an implementation of `bread_wr` which efficiently caches the old value in the disk, which can be later used when writing to the log. 

Finally, note that in redo logs, we write to the log lazily, i.e. the `write_log` function is called only during commit. However, instead of waiting till `commit`, we can now eagerly write to the log whenever a block is changed. Your task is to modify the code to make writes to the undo log in an eager manner.

Note that you are strictly **not allowed** to make any changes to the `commit` function in `log.c`, the `main` function in `main.c`, and to any of the calls to `bread_wr` in `fs.c`. Apart from this, you are allowed to create new functions, modify existing ones, add attributes to structs, etc.  

## Deliverables (Tentative)
In summary, you are required to:
- Replace the logging system in xv6 with undo logging
- Ensure your implementation does not entail extra reads from disk
- Make sure writes to logs are eager

You will be required to submit the entire xv6-step-by-step folder with your changes. 

In the xv6-step-by-step root directory, run the following commands:

```
make clean
tar czvf lab2_<entryNumber>.tar.gz *
```
 
This will create a tarball with the name, lab2_<entryNumber>.tar.gz in the same directory. Submit this tarball on Moodle. Entry number format: 2020CS10567. (All English letters will be in capitals in the
entry number)

## Auto-grader

Your submission will be evaluated by our autograder which is available [here](https://csciitd-my.sharepoint.com/:u:/g/personal/jcs222656_iitd_ac_in/EUFR-OxqFJRGrxDwG2eK8m0BPLuuo4-nQ-HH5o5_4jAzig).

- Download and unzip the check_scripts_lab2.zip.
```
unzip check_scripts_lab2.zipcd check_scripts
```

- Use the following command to run the auto-grader
```
bash check.sh <path/to/lab2_<entryNumber>.tar.gz>
```
  
- The given sample script consists of 5 basic test cases. Note that your submission will be evaluated by hidden test cases.