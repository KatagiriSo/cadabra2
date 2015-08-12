
#include "Actions.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <iostream>

using namespace cadabra;

DocumentThread::DocumentThread(GUIBase* g)
	: gui(g), compute(0)
	{
	}

void DocumentThread::set_compute_thread(ComputeThread *cl)
	{
	compute = cl;
	}

void DocumentThread::new_document()
	{
	// Setup a single-cell document. This operation itself cannot be undone,
	// so we do it directly on the doc, not using Actions.

	DataCell top(DataCell::CellType::document);
	DTree::iterator doc_it = doc.set_head(top);
	gui->add_cell(doc, doc_it, false);

	// One Python input cell in the empty document.

	DataCell one(DataCell::CellType::python, "");
	DTree::iterator one_it = doc.append_child(doc_it, one);
	gui->add_cell(doc, one_it, false);
	}

void DocumentThread::build_visual_representation()
	{
	// Because the add_cell method figures out by itself where to generate the VisualCell,
	// we only have feed all cells in turn.

	DTree::iterator doc_it=doc.begin();
	while(doc_it!=doc.end()) {
//		std::cout << doc_it->textbuf << std::endl;
		gui->add_cell(doc, doc_it, false);
		++doc_it;
		}
	}

const DTree& DocumentThread::dtree() 
	{
	return doc;
	}

void DocumentThread::queue_action(std::shared_ptr<ActionBase> ab) 
	{
	std::lock_guard<std::mutex> guard(stack_mutex);
	pending_actions.push(ab);
	}


void DocumentThread::process_action_queue()
	{
	// FIXME: we certainly do not want any two threads to run this at the same time,
	// but that is not guaranteed.

	stack_mutex.lock();
	while(pending_actions.size()>0) {
		std::shared_ptr<ActionBase> ab = pending_actions.front();
		// Unlock the queue while we are processing this particular action.
		stack_mutex.unlock();
		ab->pre_execute(*this);
		ab->update_gui(doc, *gui);
		ab->post_execute(*this);
		// Lock the queue to remove the running action.
		stack_mutex.lock();
		pending_actions.pop();
		}
	stack_mutex.unlock();
	}
