#include "single_tree_node.h"

void node_rem(node** lst, node* toRemove) {
	if (uuid_compare((*lst)->uuid, toRemove->uuid) == 0) {
		node* tmp = *lst;
		*lst = (*lst)->next;
		tmp->next = NULL;
		free(tmp);
	} else {
		node* tmp = *lst;
		while (tmp->next != NULL) {
			if (uuid_compare(tmp->next->uuid, toRemove->uuid) == 0) {
				node* next = tmp->next;
				tmp->next = next->next;
				next->next = NULL;
				free(next);
				break;

			} else {
				tmp = tmp->next;
			}
		}
	}

	free(toRemove);
}

void node_add(node** lst, node* toAdd) {
	toAdd->next = *lst;
	*lst = toAdd;
}

node* node_create(const WLANAddr src_addr, const uuid_t* id) {
	node* n = malloc(sizeof(node));

	memcpy(n->addr.data, src_addr.data, WLAN_ADDR_LEN);
	memcpy(n->uuid, id, sizeof(uuid_t));

	n->next = NULL;
	return n;
}

node* node_get_copy(node* original) {
	node* copy = NULL;
	node* tmp = original;
	if (tmp != NULL) {
		copy = malloc(sizeof(node));
		memcpy(copy->addr.data, tmp->addr.data, WLAN_ADDR_LEN);
		memcpy(copy->uuid, tmp->uuid, sizeof(uuid_t));
		tmp = tmp->next;
		copy->next = NULL;
	}
	node* prev = NULL;
	while (tmp != NULL) {
		node* tmp_cpy = malloc(sizeof(node));
		memcpy(tmp_cpy->addr.data, tmp->addr.data, WLAN_ADDR_LEN);
		memcpy(tmp_cpy->uuid, tmp->uuid, sizeof(uuid_t));
		tmp_cpy->next = NULL;
		if (copy->next == NULL)
			copy->next = tmp_cpy;
		else {
			prev->next = tmp_cpy;
		}
		prev = tmp_cpy;
		tmp = tmp->next;
	}

	return copy;
}

node* node_get_copy_with_exclude(node* original, uuid_t exclude) {
	node* copy = NULL;
	node* tmp = original;
	while (tmp != NULL && copy == NULL) {
		if (uuid_compare(tmp->uuid, exclude) != 0) {
			copy = malloc(sizeof(node));
			memcpy(copy->addr.data, tmp->addr.data, WLAN_ADDR_LEN);
			memcpy(copy->uuid, tmp->uuid, sizeof(uuid_t));
			copy->next = NULL;
		}
		tmp = tmp->next;
	}
	node* prev = NULL;
	while (tmp != NULL) {
		if (uuid_compare(tmp->uuid, exclude) != 0) {
			node* tmp_cpy = malloc(sizeof(node));
			memcpy(tmp_cpy->addr.data, tmp->addr.data, WLAN_ADDR_LEN);
			memcpy(tmp_cpy->uuid, tmp->uuid, sizeof(uuid_t));
			tmp_cpy->next = NULL;
			if (copy->next == NULL)
				copy->next = tmp_cpy;
			else {
				prev->next = tmp_cpy;
			}
			prev = tmp_cpy;
		}
		tmp = tmp->next;
	}

	return copy;
}
