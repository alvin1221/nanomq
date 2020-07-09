
#include "include/apps.h"
#include "apps/mq.h"

#include <stdlib.h>

NANOMQ_APP(mq, mqcreate_debug, mqsend_debug, mqreceive_debug);
//DASH_APP(dashboard_controller, NULL, dshbrd_controller_start, dshbrd_controller_stop);

#if defined(DASH_DEBUG)
//DASH_APP();
#endif

const struct nanomq_app *edge_apps[] = {
	//&dash_app_dashboard_controller,
	&nanomq_app_mq,
#if defined(DASH_DEBUG)
	//&
#endif
	NULL,
};
