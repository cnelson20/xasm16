struct dyn_list {
	void **list;
	unsigned short length;
	unsigned short capacity;
};

void init_dyn_list(struct dyn_list *l);
void dyn_list_add(struct dyn_list *l, void *data);