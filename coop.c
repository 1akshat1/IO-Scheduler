#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct coop_data {
	struct list_head queue;
  u64 runtime;
};

static void coop_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int coop_dispatch(struct request_queue *q, int force)
{
	struct coop_data *nd = q->elevator->elevator_data;
	struct request *rq;
	rq = list_first_entry_or_null(&nd->queue, struct request, queuelist);
	if (rq) {
     
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}
	return 0;


}

static struct request *
coop_former_request(struct request_queue *q, struct request *rq)
{
	struct coop_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_prev_entry(rq, queuelist);
}

static struct request *
coop_latter_request(struct request_queue *q, struct request *rq)
{
	struct coop_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_next_entry(rq, queuelist);
}


static void coop_add_request(struct request_queue *q, struct request *rq)
{
  u64 curr_vrun,curr_time;
  struct task_struct *tsk = current;
  struct sched_entity sched = tsk->se;
  u64 vruntime = sched.vruntime;
  u64 arrival = rdtsc();
  
	struct coop_data *nd = q->elevator->elevator_data;
   (rq->elv).priv[0] = (void *)vruntime;
   (rq->elv).priv[1] = (void *)arrival;
 
  if(list_empty(&nd->queue)){
    list_add_tail(&rq->queuelist, &nd->queue);
  }
 else{

	/*
	 * Traversing the request queue
	 */
 	int i =0;
 	struct request * first = list_first_entry(&nd->queue,struct request, queuelist );
 	struct request *latter = coop_latter_request(q, first);
 	
	while (latter){
		struct request *current_req = latter;
		i=1;
 		curr_vrun = (unsigned long)(current_req->elv).priv[0];
		curr_time = (unsigned long)(current_req->elv).priv[1];
		if((arrival/100)-(curr_time/100)<=15){
			if (vruntime <= curr_vrun ){
				list_add_tail(&rq->queuelist, &current_req->queuelist);
				i = -1;
				break;
			}
			else{
				latter = coop_latter_request(q,current_req);	
			}
		}
		else		
 		latter = coop_latter_request(q,current_req);
 	}

if (i!=-1){
	list_add_tail(&rq->queuelist, &nd->queue);
	}
} 

}




static int coop_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct coop_data *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void coop_exit_queue(struct elevator_queue *e)
{
	struct coop_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_coop = {
	.ops.sq = {
		.elevator_merge_req_fn		= coop_merged_requests,
		.elevator_dispatch_fn		= coop_dispatch,
		.elevator_add_req_fn		= coop_add_request,
		.elevator_former_req_fn		= coop_former_request,
		.elevator_latter_req_fn		= coop_latter_request,
		.elevator_init_fn		= coop_init_queue,
		.elevator_exit_fn		= coop_exit_queue,
	},
	.elevator_name = "coop",
	.elevator_owner = THIS_MODULE,
};

static int __init coop_init(void)
{
	return elv_register(&elevator_coop);
}

static void __exit coop_exit(void)
{
	elv_unregister(&elevator_coop);
}

module_init(coop_init);
module_exit(coop_exit);


MODULE_AUTHOR("Sachin Yadav");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Co-op IO scheduler");
