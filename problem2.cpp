
// measure atmospheric temperature
// multicore CPU
// 8 temp sensors:
// - collect temp readings @ regular intervals
// - store temp readings in shared memory space
// atmospheric temp module: 
// - compile report @ end of ever hr
//   * top 5 lowest temps recorded during hr
//   * 10 min interval time largest temp diff observed
// data storage & retrieval carefully handled
// - don't want to delay sensor/miss interval of time supposed to read
// design:
// - 8 threads offering a solution
// - assume temp readings every 1 min
// - simulate operation of temp reading sensor by:
//   * gen random num from -100F to 70 F every reading

