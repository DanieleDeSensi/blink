class wl_manager:
    def write_script(self,wlm_path,runner_args,schedules,nams,name,splits,node_file,ppn):
        #not needed if runner.py is run directly (runner.py doesn't call this, driver.py does)
        #write script with name name
        #that calls 'python3 runner.py' with specified arguments

    def schedule_job(self,node_list,ppn,cmd):
        #return string of command that can be called in shell to run the specified cmd on nodes in nodelist with ppn

