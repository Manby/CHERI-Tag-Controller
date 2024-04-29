cmake . &&
make &&
if [ -z $2 ]; then
  ./bin/cheri-tag-controller-verifier $1 /media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_initial_state.bin /media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_llc_requests.gz
else
  ./bin/cheri-tag-controller-verifier $1 /media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_initial_state.bin /media/Omega/University/Part-II/Project/traces/trace_2023-09-04_22-13-45_507213/trace_llc_requests.gz $2
fi